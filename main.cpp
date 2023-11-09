#include <iostream>
#include <fstream>
#include <iomanip>
#include <string>
#include <sstream>
#include <curl/curl.h>
#include <nlohmann/json.hpp>

size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t total_size = size * nmemb;
    output->append(static_cast<char*>(contents), total_size);
    return total_size;
}

double roundToHalfHour(double seconds) {
    double hours = seconds / 3600.0;
    double roundedHours = round(hours * 2.0) / 2.0;

    if (std::floor(roundedHours) == roundedHours) {
        return static_cast<int>(roundedHours);
    } else {
        return std::round(roundedHours * 10.0) / 10.0;
    }
}

std::string formatUTCDate(const std::string& utcDateString) {
    std::tm currentTM = {};
    std::istringstream ss(utcDateString);

    // Parse the input UTC date string
    ss >> std::get_time(&currentTM, "%Y-%m-%dT%H:%M:%S");

    if (ss.fail()) {
        return "";
    }

    // Format the date as "YYYY-MM-DD"
    std::stringstream formattedDate;
    formattedDate << std::setw(4) << std::setfill('0') << (currentTM.tm_year + 1900) << "-";
    formattedDate << std::setw(2) << std::setfill('0') << (currentTM.tm_mon + 1) << "-";
    formattedDate << std::setw(2) << std::setfill('0') << currentTM.tm_mday;

    return formattedDate.str();
}

std::time_t convertDateToUnixTime(const std::string& dateString) {
    // Parse the input date string
    std::tm timeStruct = {};
    std::istringstream dateStream(dateString);
    dateStream >> std::get_time(&timeStruct, "%Y-%m-%d");

    if (dateStream.fail()) {
        std::cerr << "Failed to parse the date string." << std::endl;
        return -1; // Return an error value
    }

    // Convert std::tm to std::chrono::system_clock::time_point
    auto timePoint = std::chrono::system_clock::from_time_t(std::mktime(&timeStruct));

    // Convert std::chrono::system_clock::time_point to Unix time
    return std::chrono::duration_cast<std::chrono::seconds>(timePoint.time_since_epoch()).count();
}

// Function to perform an HTTP GET request
nlohmann::json performHttpGetRequest(const std::string& endpoint, const std::string& username, const std::string& password) {

    CURL* curl = curl_easy_init();

    if (!curl) {
        return "Error initializing libcurl.";
    }

    std::string auth_string = username + ":" + password;
    curl_easy_setopt(curl, CURLOPT_USERPWD, auth_string.c_str());

    std::string response;

    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    curl_easy_setopt(curl, CURLOPT_URL, endpoint.c_str());

    CURLcode res = curl_easy_perform(curl);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        return "Error performing the request: " + std::string(curl_easy_strerror(res));
    }

    curl_easy_cleanup(curl);
    return nlohmann::json::parse(response);
}

int main() {

    bool hasFetched = false;

    while (!hasFetched) {

        std::string endpoint = "https://api.track.toggl.com/api/v9/me/time_entries";
        std::string username;
        std::string password;
        std::string start_date;
        std::string end_date;

        std::cout << "Enter username and password, then press enter." << std::endl;

        std::cout << "Username: ";
        std::cin >> username;

        std::cout << "Password: ";
        std::cin >> password;

        std::cout << "Date from (YYYY-MM-DD): ";
        std::cin >> start_date;

        std::cout << "Date to (YYYY-MM-DD): ";
        std::cin >> end_date;

        std::cout << "\nThanks, preparing your export. Please wait..." << std::endl;

        endpoint += "?start_date=";
        endpoint += start_date;
        endpoint += "&end_date=";
        endpoint += end_date;

        nlohmann::json toggleResponse = performHttpGetRequest(endpoint, username, password);

        if (!toggleResponse.is_null()) {

            std::ofstream csvFile("togglExport.csv");

            csvFile << "Datum, Artikel, Tid(timmar), Kund, Projekt, Aktivitet, Ärendenummer, Beskrivning" << std::endl;

            for (const auto &entry: toggleResponse) {

                long workspace_id;
                std::string workspace_name;
                if (!entry["workspace_id"].is_null()) {
                    workspace_id = entry["workspace_id"];
                    std::string workspace_id_string = std::to_string(workspace_id);
                    std::string workspaceEndpoint = "https://api.track.toggl.com/api/v9/workspaces/";
                }

                // Project id (needs to be null checked)
                long project_id;
                std::string project_name;
                std::string client_name;

                if (!entry["project_id"].is_null()) {
                    project_id = entry["project_id"];
                    std::string project_id_string = std::to_string(project_id);
                    std::string projectsEndpoint = "https://api.track.toggl.com/api/v9/workspaces/";
                    projectsEndpoint += std::to_string(workspace_id);
                    projectsEndpoint += "/projects/";
                    projectsEndpoint += std::to_string(project_id);

                    nlohmann::json projectResponse = performHttpGetRequest(projectsEndpoint, username, password);

                    project_name = projectResponse["name"].get<std::string>();
                    auto client_id = projectResponse["client_id"].get<int>();

                    projectsEndpoint = "https://api.track.toggl.com/api/v9/workspaces/";
                    projectsEndpoint += std::to_string(workspace_id);
                    projectsEndpoint += "/clients/";
                    projectsEndpoint += std::to_string(client_id);

                    client_name = performHttpGetRequest(projectsEndpoint, username,
                                                        password)["name"].get<std::string>();

                }

                double duration = entry["duration"];
                std::string start_time = entry["start"];
                std::string description = entry["description"];
                std::string activity;

                if (!entry["tags"].empty()) {
                    activity = entry["tags"][0];
                }

                // Date in correct format
                csvFile <<
                        formatUTCDate(start_time) + ","
                        + "Normal" + ","
                        + std::to_string(roundToHalfHour(duration)) + ","
                        + client_name + ","
                        + project_name + ","
                        + activity + ","
                        + description + ","
                        + "" + ","
                        << std::endl;

            }

            csvFile.close();
        }
        std::cout << "Export is completed." << std::endl;
        hasFetched = true;
    }

    return 0;

}
