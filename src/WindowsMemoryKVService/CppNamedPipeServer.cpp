#include "NamedPipeServer.h"

void NamedPipeServer::backgroundTaskHandler()
{
    while (running) {
        std::unique_lock<std::mutex> lock(taskMutex);
        cv.wait_for(lock,
                    std::chrono::seconds(5),
                    [&] { return !taskList.empty(); }
        );

        if (!taskList.empty()) {
            std::string task = taskList.back();
            taskList.pop_back();

            lock.unlock();

            // Simulate task processing
            std::cout << "Processing task: " << task << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(2));  // Simulate task processing time
            std::cout << "Task completed: " << task << std::endl;
        }
    }
}

void NamedPipeServer::processClientRequests(HANDLE hPipe)
{
    char buffer[128];
    DWORD bytesRead;

    while (running) {
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

        if (!success || bytesRead == 0) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_BROKEN_PIPE) {
                std::cerr << "Client disconnected." << std::endl;
                break;
            }
            std::cerr << "Error reading from pipe. Error code: " << dwError << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';  // Null-terminate the string
        std::string request(buffer);

        std::cout << "Received request: " << request << std::endl;

        if (request == "EXIT") {
            std::cout << "Shutting down server." << std::endl;
            running = false;
            break;
        }
        else if (request.rfind("ADD ", 0) == 0) {
            // Add task
            std::string task = request.substr(4);
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                taskList.push_back(task);
                std::cout << "Task added: " << task << std::endl;
            }
            cv.notify_all();
        }
        else if (request.rfind("REMOVE ", 0) == 0) {
            // Remove task
            std::string task = request.substr(7);
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                auto it = std::find(taskList.begin(), taskList.end(), task);
                if (it != taskList.end()) {
                    taskList.erase(it);
                    std::cout << "Task removed: " << task << std::endl;
                }
                else {
                    std::cout << "Task not found: " << task << std::endl;
                }
            }
        }
        else {
            std::cout << "Unknown request: " << request << std::endl;
        }
    }
}

int NamedPipeServer::Serve()
{
    HANDLE hPipe;

    // Create named pipe
    hPipe = CreateNamedPipe(
        PIPE_NAME,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        1,
        512,
        512,
        0,
        NULL);

    if (hPipe == INVALID_HANDLE_VALUE) {
        DWORD dwError = GetLastError();
        std::cerr << "Failed to create named pipe. Error code: " << dwError << std::endl;
        if (dwError == ERROR_ACCESS_DENIED) {
            std::cerr << "Access denied. Try running the program as Administrator." << std::endl;
        }
        else if (dwError == ERROR_PIPE_BUSY) {
            std::cerr << "Named pipe is busy. The pipe may already be in use by another process." << std::endl;
        }
        return 1;
    }

    // Start background task handler thread
    std::thread backgroundThread(&NamedPipeServer::backgroundTaskHandler, this);

    std::cout << "Server is waiting for client connection..." << std::endl;

    // Wait for client connection
    auto connected = ConnectNamedPipe(hPipe, NULL);
    if (!connected) {
        DWORD dwError = GetLastError();
        std::cerr << "Failed to connect to client. Error code: " << dwError << std::endl;
        CloseHandle(hPipe);
        return 1;
    }

    std::cout << "Client connected." << std::endl;

    char buffer[128];
    DWORD bytesRead;
    while (running) {
        BOOL success = ReadFile(hPipe, buffer, sizeof(buffer), &bytesRead, NULL);

        if (!success || bytesRead == 0) {
            DWORD dwError = GetLastError();
            if (dwError == ERROR_BROKEN_PIPE) {
                std::cerr << "Client disconnected." << std::endl;
                break;
            }
            std::cerr << "Error reading from pipe. Error code: " << dwError << std::endl;
            break;
        }

        buffer[bytesRead] = '\0';  // Null-terminate the string
        std::string request(buffer);

        std::cout << "Received request: " << request << std::endl;

        if (request == "EXIT") {
            std::cout << "Shutting down server." << std::endl;
            running = false;
            break;
        }
        else if (request.rfind("ADD ", 0) == 0) {
            // Add task
            std::string task = request.substr(4);
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                taskList.push_back(task);
                std::cout << "Task added: " << task << std::endl;
            }
            cv.notify_all();
        }
        else if (request.rfind("REMOVE ", 0) == 0) {
            // Remove task
            std::string task = request.substr(7);
            {
                std::lock_guard<std::mutex> lock(taskMutex);
                auto it = std::find(taskList.begin(), taskList.end(), task);
                if (it != taskList.end()) {
                    taskList.erase(it);
                    std::cout << "Task removed: " << task << std::endl;
                }
                else {
                    std::cout << "Task not found: " << task << std::endl;
                }
            }
        }
        else {
            std::cout << "Unknown request: " << request << std::endl;
        }
    }

    // Clean up
    running = false;
    backgroundThread.join();
    CloseHandle(hPipe);

    return 0;
}
