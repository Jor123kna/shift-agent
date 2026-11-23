#include "scheduler.hpp"

#include <nlohmann/json.hpp>

#include <exception>
#include <iostream>
#include <stdexcept>
#include <string>

using json = nlohmann::json;

// Only protocol messages go to standard output.
void sendResult(const json& id, const json& result) {
    json response = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"result", result}
    };

    std::cout << response.dump() << std::endl;
}

void sendError(const json& id, int code, const std::string& message) {
    json response = {
        {"jsonrpc", "2.0"},
        {"id", id},
        {"error", {
            {"code", code},
            {"message", message}
        }}
    };

    std::cout << response.dump() << std::endl;
}

json textResult(const std::string& text, bool isError = false) {
    return {
        {"content", json::array({
            {
                {"type", "text"},
                {"text", text}
            }
        })},
        {"isError", isError}
    };
}

json getTools() {
    return json::parse(R"(
    {
      "tools": [
        {
          "name": "list_employees",
          "description": "List employee IDs and names.",
          "inputSchema": {
            "type": "object",
            "properties": {},
            "additionalProperties": false
          }
        },
        {
          "name": "list_shifts",
          "description": "List shifts for the demonstration week. Days use Monday=1 through Sunday=7. Employee ID -1 means unassigned.",
          "inputSchema": {
            "type": "object",
            "properties": {},
            "additionalProperties": false
          }
        },
        {
          "name": "find_available_employees",
          "description": "Find employees who can work an unassigned shift without availability conflicts or overlapping assignments.",
          "inputSchema": {
            "type": "object",
            "properties": {
              "shiftId": {
                "type": "integer",
                "minimum": 1
              }
            },
            "required": ["shiftId"],
            "additionalProperties": false
          }
        },
        {
          "name": "assign_shift",
          "description": "Assign an employee to a shift and save the schedule. Rejects unavailable employees, overlaps, and already assigned shifts. Returns a simulated email.",
          "inputSchema": {
            "type": "object",
            "properties": {
              "employeeId": {
                "type": "integer",
                "minimum": 1
              },
              "shiftId": {
                "type": "integer",
                "minimum": 1
              }
            },
            "required": ["employeeId", "shiftId"],
            "additionalProperties": false
          }
        }
      ]
    }
    )");
}

// Validate arguments before passing them to the scheduler.
void validateArguments(
    const json& arguments,
    const json& requiredNames
) {
    if (!arguments.is_object()) {
        throw std::runtime_error("Arguments must be an object.");
    }

    if (arguments.size() != requiredNames.size()) {
        throw std::runtime_error("Missing or unexpected arguments.");
    }

    for (const auto& item : requiredNames) {
        std::string name = item.get<std::string>();

        if (!arguments.contains(name) ||
            !arguments.at(name).is_number_integer()) {
            throw std::runtime_error(
                name + " must be a whole number."
            );
        }

        int value = arguments.at(name).get<int>();

        if (value < 1) {
            throw std::runtime_error(name + " must be positive.");
        }
    }
}

json runTool(Scheduler& scheduler, const json& params) {
    std::string name = params.at("name").get<std::string>();
    json arguments = params.value("arguments", json::object());

    if (name == "list_employees") {
        validateArguments(arguments, json::array());

        json employees = json::array();

        for (const Employee& employee : scheduler.getEmployees()) {
            employees.push_back({
                {"id", employee.getId()},
                {"name", employee.getName()}
            });
        }

        return textResult(employees.dump());
    }

    if (name == "list_shifts") {
        validateArguments(arguments, json::array());

        json shifts = json::array();

        for (const Shift& shift : scheduler.getShifts()) {
            shifts.push_back({
                {"id", shift.getId()},
                {"day", shift.getDay()},
                {"startHour", shift.getStartHour()},
                {"endHour", shift.getEndHour()},
                {"employeeId", shift.getEmployeeId()}
            });
        }

        return textResult(shifts.dump());
    }

    if (name == "find_available_employees") {
        validateArguments(arguments, json::array({"shiftId"}));

        int shiftId = arguments.at("shiftId").get<int>();
        json available = json::array();

        for (const Employee& employee :
             scheduler.findAvailableEmployees(shiftId)) {
            available.push_back({
                {"id", employee.getId()},
                {"name", employee.getName()}
            });
        }

        return textResult(available.dump());
    }

    if (name == "assign_shift") {
        validateArguments(
            arguments,
            json::array({"employeeId", "shiftId"})
        );

        return textResult(scheduler.assignShift(
            arguments.at("employeeId").get<int>(),
            arguments.at("shiftId").get<int>()
        ));
    }

    throw std::runtime_error("Unknown tool: " + name);
}

void runServer(Scheduler& scheduler) {
    bool initializationStarted = false;
    bool ready = false;
    std::string line;

    while (std::getline(std::cin, line)) {
        json request;

        try {
            request = json::parse(line);
        } catch (const json::parse_error&) {
            sendError(nullptr, -32700, "Invalid JSON.");
            continue;
        }

        if (!request.is_object() ||
            request.value("jsonrpc", json()) != "2.0" ||
            !request.contains("method") ||
            !request.at("method").is_string()) {
            sendError(nullptr, -32600, "Invalid JSON-RPC request.");
            continue;
        }

        std::string method = request.at("method").get<std::string>();
        bool hasId = request.contains("id");
        json id = request.value("id", json());

        // Notifications do not receive responses.
        if (!hasId) {
            if (method == "notifications/initialized" &&
                initializationStarted) {
                ready = true;
            }
            continue;
        }

        if (!id.is_string() && !id.is_number_integer()) {
            sendError(nullptr, -32600, "Invalid request ID.");
            continue;
        }

        try {
            json params = request.value("params", json::object());

            if (!params.is_object()) {
                sendError(id, -32602, "Params must be an object.");
                continue;
            }

            if (method == "initialize") {
                if (initializationStarted) {
                    sendError(id, -32600, "Already initialized.");
                    continue;
                }

                if (!params.contains("protocolVersion") ||
                    !params.at("protocolVersion").is_string() ||
                    !params.contains("capabilities") ||
                    !params.at("capabilities").is_object() ||
                    !params.contains("clientInfo") ||
                    !params.at("clientInfo").is_object()) {
                    sendError(id, -32602, "Invalid initialization parameters.");
                    continue;
                }

                sendResult(id, {
                    {"protocolVersion", "2025-06-18"},
                    {"capabilities", {
                        {"tools", {
                            {"listChanged", false}
                        }}
                    }},
                    {"serverInfo", {
                        {"name", "shift-agent"},
                        {"version", "1.0.0"}
                    }}
                });

                initializationStarted = true;
            } else if (method == "ping") {
                sendResult(id, json::object());
            } else if (!ready) {
                sendError(id, -32600, "Initialize the connection first.");
            } else if (method == "tools/list") {
                sendResult(id, getTools());
            } else if (method == "tools/call") {
                if (!params.contains("name") ||
                    !params.at("name").is_string()) {
                    sendError(id, -32602, "A tool name is required.");
                    continue;
                }

                try {
                    sendResult(id, runTool(scheduler, params));
                } catch (const std::exception& error) {
                    sendResult(id, textResult(error.what(), true));
                }
            } else {
                sendError(id, -32601, "Method not found.");
            }
        } catch (const std::exception& error) {
            sendError(id, -32602, error.what());
        }
    }
}

int main() {
    try {
        Scheduler scheduler(
            "data/employees.json",
            "data/shifts.json"
        );

        runServer(scheduler);
    } catch (const std::exception& error) {
        // Diagnostics go to stderr, not stdout.
        std::cerr << "Could not start MCP server: "
                  << error.what() << '\n';
        return 1;
    }

    return 0;
}