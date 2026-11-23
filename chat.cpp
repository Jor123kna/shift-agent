#include <curl/curl.h>
#include <nlohmann/json.hpp>

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

using json = nlohmann::json;

// Receives the HTTP response from llama.cpp.
size_t receiveResponse(
    char* data,
    size_t size,
    size_t count,
    void* destination
) {
    size_t bytes = size * count;
    auto* response = static_cast<std::string*>(destination);

    response->append(data, bytes);
    return bytes;
}

json askModel(const json& messages, const json& tools) {
    CURL* curl = curl_easy_init();

    if (curl == nullptr) {
        throw std::runtime_error("Could not initialize HTTP client.");
    }

    json request = {
        {"model", "local-model"},
        {"messages", messages},
        {"tools", tools},
        {"tool_choice", "auto"},
        {"parallel_tool_calls", false},
        {"temperature", 0.1},
        {"max_tokens", 1024},
        {"stream", false},
        {"chat_template_kwargs", {
            {"enable_thinking", false}
        }}
    };

    std::string body = request.dump();
    std::string response;

    curl_slist* headers = nullptr;
    headers = curl_slist_append(
        headers,
        "Content-Type: application/json"
    );

    curl_easy_setopt(
        curl,
        CURLOPT_URL,
        "http://127.0.0.1:9931/v1/chat/completions"
    );
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, body.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, receiveResponse);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

    CURLcode result = curl_easy_perform(curl);

    long status = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result != CURLE_OK) {
        throw std::runtime_error(
            std::string("Cannot contact llama.cpp: ") +
            curl_easy_strerror(result)
        );
    }

    if (status != 200) {
        throw std::runtime_error(
            "llama.cpp returned HTTP " +
            std::to_string(status) + ": " + response
        );
    }

    return json::parse(response)
        .at("choices").at(0).at("message");
}

class McpClient {
private:
    pid_t processId = -1;
    FILE* input = nullptr;  // Write requests to the server.
    FILE* output = nullptr; // Read responses from the server.
    int nextId = 1;

    void send(const json& message) {
        std::string line = message.dump() + "\n";

        if (std::fputs(line.c_str(), input) == EOF ||
            std::fflush(input) != 0) {
            throw std::runtime_error("Could not send MCP message.");
        }
    }

    json readMessage() {
        // POSIX getline can read a complete JSON line of any size.
        char* buffer = nullptr;
        size_t capacity = 0;

        ssize_t length = ::getline(&buffer, &capacity, output);

        if (length < 0) {
            std::free(buffer);
            throw std::runtime_error(
                "MCP server closed the connection. "
                "Check its error messages and data files."
            );
        }

        std::string line(buffer, static_cast<size_t>(length));
        std::free(buffer);

        return json::parse(line);
    }

public:
    McpClient() {
        int toServer[2];
        int fromServer[2];

        if (pipe(toServer) == -1) {
            throw std::runtime_error("Could not create MCP pipe.");
        }

        if (pipe(fromServer) == -1) {
            close(toServer[0]);
            close(toServer[1]);
            throw std::runtime_error("Could not create MCP pipe.");
        }

        processId = fork();

        if (processId == -1) {
            close(toServer[0]);
            close(toServer[1]);
            close(fromServer[0]);
            close(fromServer[1]);
            throw std::runtime_error("Could not start MCP process.");
        }

        if (processId == 0) {
            // Connect the child's stdin/stdout to our pipes.
            if (dup2(toServer[0], STDIN_FILENO) == -1 ||
                dup2(fromServer[1], STDOUT_FILENO) == -1) {
                _exit(1);
            }

            close(toServer[0]);
            close(toServer[1]);
            close(fromServer[0]);
            close(fromServer[1]);

            execl("./shift-mcp", "shift-mcp", nullptr);

            // Runs only if launching the executable failed.
            std::perror("Could not launch ./shift-mcp");
            _exit(1);
        }

        close(toServer[0]);
        close(fromServer[1]);

        input = fdopen(toServer[1], "w");
        output = fdopen(fromServer[0], "r");

        if (input == nullptr || output == nullptr) {
            if (input != nullptr) {
                std::fclose(input);
            } else {
                close(toServer[1]);
            }

            if (output != nullptr) {
                std::fclose(output);
            } else {
                close(fromServer[0]);
            }

            waitpid(processId, nullptr, 0);
            throw std::runtime_error("Could not open MCP streams.");
        }
    }

    // Pipes belong to this object and should not be copied.
    McpClient(const McpClient&) = delete;
    McpClient& operator=(const McpClient&) = delete;

    ~McpClient() {
        if (input != nullptr) {
            std::fclose(input);
        }

        if (output != nullptr) {
            std::fclose(output);
        }

        if (processId > 0) {
            waitpid(processId, nullptr, 0);
        }
    }

    json request(const std::string& method, const json& params) {
        int id = nextId++;

        send({
            {"jsonrpc", "2.0"},
            {"id", id},
            {"method", method},
            {"params", params}
        });

        while (true) {
            json response = readMessage();

            // Ignore notifications while waiting for our response.
            if (!response.contains("id") ||
                response.at("id") != id) {
                continue;
            }

            if (response.contains("error")) {
                throw std::runtime_error(
                    response.at("error").dump()
                );
            }

            return response.at("result");
        }
    }

    void initialize() {
        json result = request("initialize", {
            {"protocolVersion", "2025-06-18"},
            {"capabilities", json::object()},
            {"clientInfo", {
                {"name", "shift-chat"},
                {"version", "1.0.0"}
            }}
        });

        if (result.at("protocolVersion") != "2025-06-18") {
            throw std::runtime_error("Unsupported MCP version.");
        }

        send({
            {"jsonrpc", "2.0"},
            {"method", "notifications/initialized"}
        });
    }
};

void runChat(McpClient& mcp) {
    json discovered = mcp.request("tools/list", json::object());
    json tools = json::array();

    // Convert MCP tool descriptions to the model's tool format.
    for (const auto& tool : discovered.at("tools")) {
        tools.push_back({
            {"type", "function"},
            {"function", {
                {"name", tool.at("name")},
                {"description", tool.at("description")},
                {"parameters", tool.at("inputSchema")}
            }}
        });
    }

    std::cout << "Connected to MCP. Discovered "
              << tools.size() << " tools.\n";

    const json systemMessage = {
        {"role", "system"},
        {"content",
            "You help schedule employees for one demonstration week. "
            "Use tools to get employee information and current shifts. "
            "Use assign_shift for assignment requests. "
            "Never claim a shift changed unless the tool succeeded. "
            "Employee ID -1 means unassigned. "
            "Days are Monday=1 through Sunday=7. "
            "Ask for clarification when a request is ambiguous. "
            "Report tool errors accurately. "
            "Emails are simulated; no real email is sent."
        }
    };

    json messages = json::array({systemMessage});
    std::string userInput;

    std::cout << "Type /reset to clear chat history, or /quit to exit.\n";

    while (true) {
        std::cout << "\nYou: ";

        if (!std::getline(std::cin, userInput) ||
            userInput == "/quit") {
            break;
        }

        if (userInput == "/reset") {
            messages = json::array({systemMessage});
            std::cout << "Chat history cleared. Schedule unchanged.\n";
            continue;
        }

        if (userInput.empty()) {
            continue;
        }

        messages.push_back({
            {"role", "user"},
            {"content", userInput}
        });

        bool answered = false;

        // Limit repeated tool requests for a single user message.
        for (int step = 0; step < 8; ++step) {
            json reply = askModel(messages, tools);

            json assistant = {
                {"role", "assistant"},
                {"content", reply.value("content", json(nullptr))}
            };

            bool hasCalls =
                reply.contains("tool_calls") &&
                reply.at("tool_calls").is_array() &&
                !reply.at("tool_calls").empty();

            if (!hasCalls) {
                messages.push_back(assistant);

                if (reply.contains("content") &&
                    reply.at("content").is_string()) {
                    std::cout << "\nAgent: "
                              << reply.at("content").get<std::string>()
                              << '\n';
                } else {
                    std::cout << "\nAgent returned no text.\n";
                }

                answered = true;
                break;
            }

            assistant["tool_calls"] = reply.at("tool_calls");
            messages.push_back(assistant);

            for (const auto& call : reply.at("tool_calls")) {
                std::string name =
                    call.at("function").at("name").get<std::string>();

                std::cout << "[Calling " << name << "]\n";

                json result;

                try {
                    json arguments = json::parse(
                        call.at("function")
                            .at("arguments").get<std::string>()
                    );

                    result = mcp.request("tools/call", {
                        {"name", name},
                        {"arguments", arguments}
                    });
                } catch (const std::exception& error) {
                    result = {
                        {"isError", true},
                        {"content", json::array({
                            {
                                {"type", "text"},
                                {"text", error.what()}
                            }
                        })}
                    };
                }

                // Show evidence of the actual tool result.
                std::cout << "[Tool result] " << result.dump() << '\n';

                messages.push_back({
                    {"role", "tool"},
                    {"tool_call_id", call.at("id")},
                    {"content", result.dump()}
                });
            }
        }

        if (!answered) {
            std::cout << "Stopped after too many tool steps. "
                      << "Please try a smaller request.\n";
        }
    }
}

int main() {
    if (curl_global_init(CURL_GLOBAL_DEFAULT) != CURLE_OK) {
        std::cerr << "Could not initialize HTTP support.\n";
        return 1;
    }

    int exitCode = 0;

    try {
        McpClient mcp;
        mcp.initialize();
        runChat(mcp);
    } catch (const std::exception& error) {
        std::cerr << "Error: " << error.what() << '\n';
        exitCode = 1;
    }

    curl_global_cleanup();
    return exitCode;
}