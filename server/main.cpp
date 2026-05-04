#include <boost/asio.hpp>
#include "json.hpp"

#include <hiredis/hiredis.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>

using boost::asio::ip::tcp;
using json = nlohmann::json;

using RedisContextPtr = std::unique_ptr<redisContext, void(*)(redisContext*)>;
using RedisReplyPtr = std::unique_ptr<redisReply, void(*)(void*)>;

static std::string envOr(const char* name, const std::string& fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }
    return value;
}

static int envIntOr(const char* name, int fallback)
{
    const char* value = std::getenv(name);
    if (value == nullptr || std::string(value).empty()) {
        return fallback;
    }
    return std::stoi(value);
}

static unsigned short getServerPort()
{
    const char* serverPort = std::getenv("SERVER_PORT");
    if (serverPort != nullptr && std::string(serverPort).size() > 0) {
        return static_cast<unsigned short>(std::stoi(serverPort));
    }

    const char* port = std::getenv("PORT");
    if (port != nullptr && std::string(port).size() > 0) {
        return static_cast<unsigned short>(std::stoi(port));
    }

    return 5555;
}

class ParkingDatabase {
public:
    explicit ParkingDatabase(std::string seedFilePath)
        : seedFilePath_(std::move(seedFilePath)),
          redisHost_(envOr("REDISHOST", "127.0.0.1")),
          redisPort_(envIntOr("REDISPORT", 6379)),
          redisUser_(envOr("REDISUSER", "")),
          redisPassword_(envOr("REDISPASSWORD", "")),
          redisKey_(envOr("REDIS_KEY", "erkenly:snapshot"))
    {
        ensureSeeded();
    }

    void reloadFromDisk()
    {
        // Kept so the old request handler does not need to change.
        // Data now comes from Redis, not from disk.
        std::lock_guard<std::mutex> lock(mutex_);
        RedisContextPtr ctx = connectRedis();
        loadSnapshotUnlocked(ctx.get());
    }

    json makeSnapshotResponse()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RedisContextPtr ctx = connectRedis();

        json data = loadSnapshotUnlocked(ctx.get());

        return json{
            {"type", "snapshot"},
            {"lots", data.at("lots")}
        };
    }

    json reportSpot(const std::string& lotId, const std::string& spotId, bool occupied)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RedisContextPtr ctx = connectRedis();

        json data = loadSnapshotUnlocked(ctx.get());

        if (!data.contains("lots") || !data["lots"].is_array()) {
            return makeError("database is invalid");
        }

        for (auto& lot : data["lots"]) {
            if (!lot.contains("id") || !lot["id"].is_string()) {
                continue;
            }

            if (lot["id"].get<std::string>() != lotId) {
                continue;
            }

            if (!lot.contains("spots") || !lot["spots"].is_array()) {
                return makeError("lot has no spots array");
            }

            for (auto& spot : lot["spots"]) {
                if (!spot.contains("id") || !spot["id"].is_string()) {
                    continue;
                }

                if (spot["id"].get<std::string>() == spotId) {
                    spot["occupied"] = occupied;
                    saveSnapshotUnlocked(ctx.get(), data);

                    return json{
                        {"type", "ack"},
                        {"ok", true},
                        {"message", "spot updated"},
                        {"lotId", lotId},
                        {"spotId", spotId},
                        {"occupied", occupied}
                    };
                }
            }

            return makeError("spot not found");
        }

        return makeError("lot not found");
    }

private:
    std::string seedFilePath_;
    std::string redisHost_;
    int redisPort_;
    std::string redisUser_;
    std::string redisPassword_;
    std::string redisKey_;

    std::mutex mutex_;

    static json makeError(const std::string& message)
    {
        return json{
            {"type", "error"},
            {"message", message}
        };
    }

    RedisContextPtr connectRedis() const
    {
        timeval timeout{};
        timeout.tv_sec = 5;
        timeout.tv_usec = 0;

        redisContext* raw = redisConnectWithTimeout(redisHost_.c_str(), redisPort_, timeout);

        if (raw == nullptr) {
            throw std::runtime_error("failed to allocate Redis connection");
        }

        if (raw->err) {
            std::string error = raw->errstr;
            redisFree(raw);
            throw std::runtime_error("failed to connect to Redis: " + error);
        }

        RedisContextPtr ctx(raw, redisFree);

        if (!redisPassword_.empty()) {
            RedisReplyPtr authReply(nullptr, freeReplyObject);

            if (!redisUser_.empty()) {
                authReply.reset(static_cast<redisReply*>(
                    redisCommand(ctx.get(), "AUTH %s %s", redisUser_.c_str(), redisPassword_.c_str())
                ));
            } else {
                authReply.reset(static_cast<redisReply*>(
                    redisCommand(ctx.get(), "AUTH %s", redisPassword_.c_str())
                ));
            }

            if (!authReply) {
                throw std::runtime_error("Redis AUTH failed: no reply");
            }

            if (authReply->type == REDIS_REPLY_ERROR) {
                throw std::runtime_error("Redis AUTH failed: " + std::string(authReply->str));
            }
        }

        return ctx;
    }

    void ensureSeeded()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        RedisContextPtr ctx = connectRedis();
        loadSnapshotUnlocked(ctx.get());
    }

    json loadSnapshotUnlocked(redisContext* ctx)
    {
        RedisReplyPtr reply(
            static_cast<redisReply*>(redisCommand(ctx, "GET %s", redisKey_.c_str())),
            freeReplyObject
        );

        if (!reply) {
            throw std::runtime_error("Redis GET failed: no reply");
        }

        if (reply->type == REDIS_REPLY_ERROR) {
            throw std::runtime_error("Redis GET failed: " + std::string(reply->str));
        }

        if (reply->type == REDIS_REPLY_NIL) {
            json seed = loadSeedFile();
            saveSnapshotUnlocked(ctx, seed);
            return seed;
        }

        if (reply->type != REDIS_REPLY_STRING) {
            throw std::runtime_error("Redis GET returned unexpected reply type");
        }

        json data = json::parse(std::string(reply->str, reply->len));
        validateSnapshot(data);
        return data;
    }

    void saveSnapshotUnlocked(redisContext* ctx, const json& data)
    {
        validateSnapshot(data);

        const std::string serialized = data.dump();

        RedisReplyPtr reply(
            static_cast<redisReply*>(
                redisCommand(ctx,
                             "SET %s %b",
                             redisKey_.c_str(),
                             serialized.data(),
                             serialized.size())
            ),
            freeReplyObject
        );

        if (!reply) {
            throw std::runtime_error("Redis SET failed: no reply");
        }

        if (reply->type == REDIS_REPLY_ERROR) {
            throw std::runtime_error("Redis SET failed: " + std::string(reply->str));
        }
    }

    json loadSeedFile() const
    {
        if (!std::filesystem::exists(seedFilePath_)) {
            return json{{"lots", json::array()}};
        }

        std::ifstream input(seedFilePath_);

        if (!input) {
            throw std::runtime_error("failed to open seed data.json");
        }

        json data;
        input >> data;

        validateSnapshot(data);
        return data;
    }

    static void validateSnapshot(const json& data)
    {
        if (!data.contains("lots") || !data["lots"].is_array()) {
            throw std::runtime_error("parking data must contain a lots array");
        }
    }
};

json handleRequest(const json& request, ParkingDatabase& database)
{
    if (!request.contains("type") || !request["type"].is_string()) {
        return json{{"type", "error"}, {"message", "missing or invalid type"}};
    }

    const std::string type = request["type"].get<std::string>();

    if (type == "get_snapshot") {
        database.reloadFromDisk();
        return database.makeSnapshotResponse();
    }

    if (type == "report_spot") {
        if (!request.contains("lotId") || !request["lotId"].is_string()) {
            return json{{"type", "error"}, {"message", "missing or invalid lotId"}};
        }

        if (!request.contains("spotId") || !request["spotId"].is_string()) {
            return json{{"type", "error"}, {"message", "missing or invalid spotId"}};
        }

        if (!request.contains("occupied") || !request["occupied"].is_boolean()) {
            return json{{"type", "error"}, {"message", "missing or invalid occupied"}};
        }

        return database.reportSpot(
            request["lotId"].get<std::string>(),
            request["spotId"].get<std::string>(),
            request["occupied"].get<bool>()
        );
    }

    return json{{"type", "error"}, {"message", "unknown request type"}};
}

void handleClient(tcp::socket socket, ParkingDatabase& database)
{
    try {
        std::cout << "Client connected: " << socket.remote_endpoint() << '\n';

        boost::asio::streambuf buffer;

        while (true) {
            std::size_t bytesRead = boost::asio::read_until(socket, buffer, '\n');
            (void)bytesRead;

            std::istream input(&buffer);
            std::string line;
            std::getline(input, line);

            if (line.empty()) {
                continue;
            }

            json response;

            try {
                json request = json::parse(line);
                response = handleRequest(request, database);
            } catch (const std::exception& e) {
                response = json{
                    {"type", "error"},
                    {"message", std::string("invalid json or server error: ") + e.what()}
                };
            }

            const std::string payload = response.dump() + "\n";
            boost::asio::write(socket, boost::asio::buffer(payload));
        }
    } catch (const std::exception& e) {
        std::cout << "Client disconnected/error: " << e.what() << '\n';
    }
}

int main(int argc, char* argv[])
{
    (void)argc;

    try {
        const unsigned short port = getServerPort();

        const std::filesystem::path exePath = std::filesystem::absolute(argv[0]);
        const std::filesystem::path exeDir = exePath.parent_path();
        const std::filesystem::path seedFile = exeDir / "data.json";

        ParkingDatabase database(seedFile.string());

        boost::asio::io_context ioContext;
        tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));

        std::cout << "Parking server listening on port " << port << '\n';
        std::cout << "Seed data file: " << seedFile << '\n';
        std::cout << "Using Redis host: " << envOr("REDISHOST", "127.0.0.1") << '\n';
        std::cout << "Using Redis port: " << envIntOr("REDISPORT", 6379) << '\n';

        while (true) {
            tcp::socket socket(ioContext);
            acceptor.accept(socket);

            std::thread(
                [&database](tcp::socket clientSocket) mutable {
                    handleClient(std::move(clientSocket), database);
                },
                std::move(socket)
            ).detach();
        }
    } catch (const std::exception& e) {
        std::cerr << "Server fatal error: " << e.what() << '\n';
        return 1;
    }

    return 0;
}