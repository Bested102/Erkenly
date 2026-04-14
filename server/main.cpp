#include <boost/asio.hpp>
#include "json.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

using boost::asio::ip::tcp;
using json = nlohmann::json;

class ParkingDatabase {
public:
void reloadFromDisk()
{
    std::lock_guard<std::mutex> lock(mutex_);

    if (!std::filesystem::exists(filePath_)) {
        data_ = makeDefaultData();
        saveLocked();
        return;
    }

    std::ifstream input(filePath_);
    if (!input) {
        throw std::runtime_error("failed to open data.json");
    }

    input >> data_;

    if (!data_.contains("lots") || !data_["lots"].is_array()) {
        throw std::runtime_error("data.json must contain a lots array");
    }
}
    explicit ParkingDatabase(std::string filePath)
        : filePath_(std::move(filePath))
    {
        load();
    }

    json makeSnapshotResponse() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return json{
            {"type", "snapshot"},
            {"lots", data_.at("lots")}
        };
    }

    json reportSpot(const std::string& lotId, const std::string& spotId, bool occupied)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!data_.contains("lots") || !data_["lots"].is_array()) {
            return makeError("database is invalid");
        }

        for (auto& lot : data_["lots"]) {
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
                    saveLocked();

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
    std::string filePath_;
    mutable std::mutex mutex_;
    json data_;

    static json makeDefaultData()
{
    return json{
        {"lots", json::array()}
    };
}

    static json makeError(const std::string& message)
    {
        return json{
            {"type", "error"},
            {"message", message}
        };
    }

    void load()
    {
        if (!std::filesystem::exists(filePath_)) {
            data_ = makeDefaultData();
            saveLocked();
            return;
        }

        std::ifstream input(filePath_);
        if (!input) {
            throw std::runtime_error("failed to open data.json");
        }

        input >> data_;

        if (!data_.contains("lots") || !data_["lots"].is_array()) {
            throw std::runtime_error("data.json must contain a lots array");
        }
    }

    void saveLocked()
    {
        std::ofstream output(filePath_);
        if (!output) {
            throw std::runtime_error("failed to write data.json");
        }

        output << std::setw(4) << data_ << '\n';
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
                    {"message", std::string("invalid json: ") + e.what()}
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
    try {
        const unsigned short port = 5555;

        const std::filesystem::path exePath = std::filesystem::absolute(argv[0]);
        const std::filesystem::path exeDir = exePath.parent_path();
        const std::filesystem::path dataFile = exeDir / "data.json";

        ParkingDatabase database(dataFile.string());

        boost::asio::io_context ioContext;
        tcp::acceptor acceptor(ioContext, tcp::endpoint(tcp::v4(), port));

        std::cout << "Parking server listening on port " << port << '\n';
        std::cout << "Current working directory: " << std::filesystem::current_path() << '\n';
        std::cout << "Using database file: " << dataFile << '\n';

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