/*
 *   Copyright (c) 2026 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "Curl.hpp"
#include "ss.hpp"
#include "TemporaryFile.hpp"
#include <filesystem>
#include "SysExec.hpp"
#include <stdexcept>
#include "util.hpp"
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <algorithm>
#include "Finally.hpp"
#include "Lv2Log.hpp"

static const std::filesystem::path WEB_TEMP_DIR{"/var/pipedal/web_temp"};

static bool enableLogging = true;


namespace pipedal
{

    static void logHeaders(const std::vector<std::string>&headers)
    {
        if (enableLogging)
        {
            std::ofstream os {"/tmp/PipedalCurl.log"};
            for (const auto&header: headers)
            {
                os << header << "\n";
            }
        }
    }
    static std::string getCurlExitCodeString(int exitCode)
    {
        // Translate documented curl exit codes to strings.
        // https://curl.se/libcurl/c/libcurl-errors.html
        switch (exitCode)
        {
        case 0:   return "OK"; // CURLE_OK - All fine
        case 1:   return "Unsupported protocol.";
        case 2:   return "Failed to initialize.";
        case 3:   return "URL malformed.";
        case 4:   return "Feature not built-in.";
        case 5:   return "Couldn't resolve proxy.";
        case 6:   return "Couldn't resolve host.";
        case 7:   return "Failed to connect to host.";
        case 8:   return "Weird server reply.";
        case 9:   return "Access denied to remote resource.";
        case 10:  return "FTP accept failed.";
        case 11:  return "FTP weird PASS reply.";
        case 12:  return "FTP accept timeout.";
        case 13:  return "FTP weird PASV reply.";
        case 14:  return "FTP weird 227 format.";
        case 15:  return "FTP can't get host.";
        case 16:  return "HTTP/2 error.";
        case 17:  return "FTP couldn't set type.";
        case 18:  return "Partial file transfer.";
        case 19:  return "FTP couldn't retrieve file.";
        case 21:  return "FTP quote error.";
        case 22:  return "HTTP page not retrieved.";
        case 23:  return "Write error.";
        case 25:  return "Upload failed.";
        case 26:  return "Read error.";
        case 27:  return "Out of memory.";
        case 28:  return "Operation timeout.";
        case 30:  return "FTP PORT failed.";
        case 31:  return "FTP couldn't use REST.";
        case 33:  return "Range error.";
        case 35:  return "SSL connect error.";
        case 36:  return "Bad download resume.";
        case 37:  return "FILE couldn't read file.";
        case 38:  return "LDAP cannot bind.";
        case 39:  return "LDAP search failed.";
        case 42:  return "Aborted by callback.";
        case 43:  return "Bad function argument.";
        case 45:  return "Interface failed.";
        case 47:  return "Too many redirects.";
        case 48:  return "Unknown option.";
        case 49:  return "Setopt option syntax error.";
        case 52:  return "Got nothing from server.";
        case 53:  return "SSL engine not found.";
        case 54:  return "SSL engine set failed.";
        case 55:  return "Failed sending network data.";
        case 56:  return "Failure receiving network data.";
        case 58:  return "Problem with local client certificate.";
        case 59:  return "Couldn't use specified SSL cipher.";
        case 60:  return "Peer SSL certificate or SSH fingerprint verification failed.";
        case 61:  return "Unrecognized transfer encoding.";
        case 63:  return "Maximum file size exceeded.";
        case 64:  return "Requested FTP SSL level failed.";
        case 65:  return "Send failed to rewind.";
        case 66:  return "SSL engine initialization failed.";
        case 67:  return "Login denied.";
        case 68:  return "TFTP file not found.";
        case 69:  return "TFTP permission problem.";
        case 70:  return "Remote disk full.";
        case 71:  return "TFTP illegal operation.";
        case 72:  return "TFTP unknown transfer ID.";
        case 73:  return "Remote file already exists.";
        case 74:  return "TFTP no such user.";
        case 77:  return "Problem reading SSL CA cert.";
        case 78:  return "Remote file not found.";
        case 79:  return "SSH error.";
        case 80:  return "SSL shutdown failed.";
        case 82:  return "Failed to load CRL file.";
        case 83:  return "SSL issuer check failed.";
        case 84:  return "FTP PRET command failed.";
        case 85:  return "RTSP CSeq mismatch.";
        case 86:  return "RTSP session ID mismatch.";
        case 87:  return "Unable to parse FTP file list.";
        case 88:  return "Chunk callback error.";
        case 89:  return "No connection available.";
        case 90:  return "SSL pinned public key mismatch.";
        case 91:  return "SSL invalid certificate status.";
        case 92:  return "HTTP/2 stream error.";
        case 93:  return "Recursive API call.";
        case 94:  return "Authentication error.";
        case 95:  return "HTTP/3 error.";
        case 96:  return "QUIC connection error.";
        case 97:  return "Proxy handshake error.";
        case 98:  return "SSL client certificate required.";
        case 99:  return "Unrecoverable poll error.";
        case 100: return "Value or data field too large.";
        case 101: return "ECH failed.";
        default:
            return SS("Failed with exit code " << exitCode << ".");
        }
    }

    static void throwCurlExitCode(int exitCode)
    {
        if (exitCode == 0) return;
        std::string message = getCurlExitCodeString(exitCode);
        throw std::runtime_error(SS("Server download failed. " << message));
    }

    static int checkCurlHttpResponse(const std::vector<std::string> &headers)
    {
        if (headers.size() == 0)
        {
            throw std::runtime_error("Download failed. Invalid curl response: no headers.");
        }

        const std::string&httpResponse = headers[0];
        // verify that the string starts with HTTP/, and then parse the numeric error code in the result, throwing a std::runtime_error if it doesn't parse.
        // e.g. "HTTP/2 200"
        if (!httpResponse.starts_with("HTTP/"))
        {
            throw std::runtime_error(SS("Download failed. Invalid curl response: " << httpResponse));
        }

        size_t codeStart = httpResponse.find(' ');
        if (codeStart == std::string::npos)
        {
            throw std::runtime_error(SS("Download failed. Invalid curl response: " << httpResponse));
        }
        ++codeStart;

        size_t codeEnd = httpResponse.find(' ', codeStart);
        if (codeEnd == std::string::npos)
        {
            codeEnd = httpResponse.length();
        }

        int statusCode = 0;
        try
        {
            statusCode = std::stoi(httpResponse.substr(codeStart, codeEnd - codeStart));
        }
        catch (const std::exception &)
        {
            throw std::runtime_error(SS("Download failed. Invalid curl response: " << httpResponse));
        }

        return statusCode;
    }


    int CurlGet(
        const std::string &url,
        std::vector<uint8_t> &output,
        std::vector<std::string> *headersOpt
    ) {
        TemporaryFile tempFile { WEB_TEMP_DIR};
        int rc = CurlGet(url,tempFile.Path(), headersOpt);
        if (rc == 200)
        {
            std::ifstream inputStream(tempFile.Path(), std::ios::binary);
            if (!inputStream)
            {
                throw std::runtime_error("Failed to open download file.");
            }
            inputStream.seekg(0, std::ios::end);
            std::streamsize size = inputStream.tellg();
            inputStream.seekg(0, std::ios::beg);
            output.resize(static_cast<size_t>(size));
            if (size > 0)
            {
                inputStream.read(reinterpret_cast<char *>(output.data()), size);
            }
        }
        return rc;

    }
    int CurlGet(
        const std::string&url,
        std::vector<std::string> &output,
        std::vector<std::string>*headersOpt
    )
    {
        TemporaryFile tempFile {WEB_TEMP_DIR};
        int rc = CurlGet(url,tempFile.Path(), headersOpt);

        std::ifstream inputStream(tempFile.Path(), std::ios::binary);
        if (!inputStream)
        {
            throw std::runtime_error("Failed to open download file.");
        }
        output.clear();
        std::string line;
        while (std::getline(inputStream, line))
        {
            if (!line.empty() && line.back() == '\r')
            {
                line.pop_back();
            }
            output.push_back(line);
        }
        return rc;

    }


    int CurlGet(
        const std::string &url,
        const std::filesystem::path&outputPath,
        std::vector<std::string> *headersOpt
    )
    {

        TemporaryFile headersFile { WEB_TEMP_DIR};

        std::vector<std::string> defaultHeaders;
        if (headersOpt == nullptr)
        {
            headersOpt = &defaultHeaders;
        }

        bool bResult = true;

        std::string args = SS("-s -L -D " << ShellEscape(headersFile.Path().c_str())  << " " << ShellEscape(url) << " -o " << ShellEscape(outputPath.c_str())   ) ;
        auto curlOutput = sysExecForOutput("/usr/bin/curl", args);

        if (headersOpt != nullptr)
        {
            std::ifstream headersStream(headersFile.Path());
            if (headersStream)
            {
                headersOpt->clear();
                std::string line;
                while (std::getline(headersStream, line))
                {
                    // Remove trailing carriage return if present
                    if (!line.empty() && line.back() == '\r')
                    {
                        line.pop_back();
                    }
                    if (!line.empty())
                    {
                        headersOpt->push_back(line);
                    }
                }
            }
        }

        logHeaders(*headersOpt);
        if (curlOutput.exitCode == 512)
        {
            throw std::runtime_error("Invalid curl arguments.");
        }
        if (curlOutput.exitCode != EXIT_SUCCESS && curlOutput.exitCode != 22 * 256)
        {
            if (WIFEXITED(curlOutput.exitCode))
            {
                auto exitCode = WEXITSTATUS(curlOutput.exitCode);
                throwCurlExitCode(exitCode);
                return -99;
            }
            else if (WIFSIGNALED(curlOutput.exitCode))
            {
                throw std::runtime_error("No internet access.");
            }
            else
            {
                throw std::runtime_error(SS("Unable to exec curl. (exit status = " << curlOutput.exitCode << ")"));
            }
        }


        int errorCode = checkCurlHttpResponse(*headersOpt);


        return errorCode;
    }


    static int curlExec(
        const std::string&commandPath, // path to executable. 
        const std::vector<std::string> &arguments, // commandline arguments.
        const std::function<bool (const std::string &string)> &onStdoutLine,
        std::string&stdErrOutput
    ) 
    {
        // Unexpected errors throw std::runtime_error.

        // create linux socket pairs for stdout and stderr. 
        int stdoutPipe[2] = {-1,-1};
        int stderrPipe[2] = {-1,-1};

        Finally ffPipes {
            [&stdoutPipe, &stderrPipe]() 
            {
                if (stdoutPipe[0] != -1) close(stdoutPipe[0]);
                if (stdoutPipe[1] != -1) close(stdoutPipe[1]);
                if (stderrPipe[0] != -1) close(stdoutPipe[0]);
                if (stderrPipe[1] != -1) close(stdoutPipe[1]);
            }
        };
        
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, stdoutPipe) == -1)
        {
            throw std::runtime_error("Failed to create stdout socket pair");
        }
        if (socketpair(AF_UNIX, SOCK_STREAM, 0, stderrPipe) == -1)
        {
            throw std::runtime_error("Failed to create stderr socket pair");
        }

        // exec the command with the supplied arguments
        pid_t pid = fork();
        if (pid == -1)
        {
            throw std::runtime_error("Failed to fork process");
        }
        
        if (pid == 0)
        {
            // Child process
            close(stdoutPipe[0]); // Close read end
            close(stderrPipe[0]); // Close read end
            
            // Redirect stdout and stderr
            dup2(stdoutPipe[1], STDOUT_FILENO);
            dup2(stderrPipe[1], STDERR_FILENO);
            
            close(stdoutPipe[1]);
            close(stderrPipe[1]);

            
            // stdin to /dev/null.
            int devNull = open("/dev/null", O_RDONLY);
            if (devNull == -1)
            {
                std::cerr << "Failed to open /dev/null" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (dup2(devNull, STDIN_FILENO) == -1)
            {
                std::cerr << "Failed to redirect stdin" << std::endl;
                close(devNull);
                exit(EXIT_FAILURE);
            }
            close(devNull);


            // Build argv array
            std::vector<char*> argv;
            argv.push_back(const_cast<char*>(commandPath.c_str()));
            for (const auto& arg : arguments)
            {
                argv.push_back(const_cast<char*>(arg.c_str()));
            }
            argv.push_back(nullptr);
            
            execv(commandPath.c_str(), argv.data());
            
            // If execv returns, it failed
            std::cerr << "Failed to execute: " << commandPath << std::endl;
            exit(EXIT_FAILURE);
        }
        
        // Parent process
        close(stdoutPipe[1]); // Close write end
        stdoutPipe[1] = -1;
        close(stderrPipe[1]); // Close write end
        stderrPipe[1] = -1;

        // Read from stderr and stdout. 
        // stderr output gets appended to the stdErrOutput argument. 
        // stdin output gets read line by line, and onStdOutLine is called for each line of text that's read.
        
        // Set non-blocking mode for both pipes
        fcntl(stdoutPipe[0], F_SETFL, O_NONBLOCK);
        fcntl(stderrPipe[0], F_SETFL, O_NONBLOCK);
        
        std::string stdoutBuffer;
        std::string stderrBuffer;
        
        bool stdoutOpen = true;
        bool stderrOpen = true;
        
        while (stdoutOpen || stderrOpen)
        {
            fd_set readfds;
            FD_ZERO(&readfds);
            int maxfd = -1;
            
            if (stdoutOpen)
            {
                FD_SET(stdoutPipe[0], &readfds);
                maxfd = std::max(maxfd, stdoutPipe[0]);
            }
            if (stderrOpen)
            {
                FD_SET(stderrPipe[0], &readfds);
                maxfd = std::max(maxfd, stderrPipe[0]);
            }
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int ret = select(maxfd + 1, &readfds, nullptr, nullptr, &timeout);
            
            if (ret > 0)
            {
                // Read from stdout
                if (stdoutOpen && FD_ISSET(stdoutPipe[0], &readfds))
                {
                    char buffer[4096];
                    ssize_t n = read(stdoutPipe[0], buffer, sizeof(buffer));
                    if (n > 0)
                    {
                        stdoutBuffer.append(buffer, n);
                        
                        // Process complete lines
                        size_t pos;
                        while ((pos = stdoutBuffer.find('\n')) != std::string::npos)
                        {
                            std::string line = stdoutBuffer.substr(0, pos);
                            if (!line.empty() && line.back() == '\r')
                            {
                                line.pop_back();
                            }
                            if (!onStdoutLine(line))
                            {
                                // Kill the child process and return immediately
                                // do NOT waitpid, as that will be done in the processing loop.
                                kill(pid, SIGTERM);
                            }
                            stdoutBuffer.erase(0, pos + 1);
                        }
                    }
                    else if (n == 0)
                    {
                        stdoutOpen = false;
                        close(stdoutPipe[0]);
                        
                        // Process any remaining data
                        if (!stdoutBuffer.empty())
                        {
                            if (!stdoutBuffer.empty() && stdoutBuffer.back() == '\r')
                            {
                                stdoutBuffer.pop_back();
                            }
                            if (!stdoutBuffer.empty())
                            {
                                if (!onStdoutLine(stdoutBuffer))
                                {
                                    // abort  the child process 
                                    kill(pid, SIGINT); // Curl: SIGINT= graceful shutdown. SIGTERM=abrupt shutdown.
                                    // but wait for output conforming the cancellation
                                    // i.e. don't waitpid -- that will be done on cleanup, and don't abort the loop.
                                }
                            }
                        }
                    }
                }
                
                // Read from stderr
                if (stderrOpen && FD_ISSET(stderrPipe[0], &readfds))
                {
                    char buffer[4096];
                    ssize_t n = read(stderrPipe[0], buffer, sizeof(buffer));
                    if (n > 0)
                    {
                        stdErrOutput.append(buffer, n);
                    }
                    else if (n == 0)
                    {
                        stderrOpen = false;
                        close(stderrPipe[0]);
                    }
                }
            }
        }

        // Return the procesess's exit code. 
        int status;
        waitpid(pid, &status, 0);
        
        if (WIFEXITED(status))
        {
            auto exitCode = WEXITSTATUS(status);
            if (exitCode == EXIT_SUCCESS) {
                return EXIT_SUCCESS;
            }
            throwCurlExitCode(exitCode);
            return exitCode;
        }
        else if (WIFSIGNALED(status))
        {
            return -1;
        }
        
        return -2;
    }
    int CurlGet(
        const std::vector<CurlDownloadRequest> &request, 
        const std::function<void(size_t completed, size_t total)> &progressCallback,
        std::vector<std::string>*headersOpt
    )
    {
        if (request.empty())
        {
            return 200;
        }

        size_t currentDownload = 0;
        size_t totalDownloads = request.size();
        int lastStatusCode = 200;
        
        // Create temporary file for curl config
        TemporaryFile configFile{WEB_TEMP_DIR};
        
        // Write curl config file with URL and output pairs
        {
            std::ofstream configStream(configFile.Path());
            if (!configStream)
            {
                throw std::runtime_error("Failed to create curl config file");
            }
            
            for (const auto& req : request)
            {
                configStream << "url = \"" << req.url << "\"\n";
                configStream << "output = \"" << req.outputFile.string() << "\"\n";
            }
        }
        
        // Build curl arguments
        std::vector<std::string> curlArgs;
        curlArgs.push_back("-s");  // Silent mode
        curlArgs.push_back("-L");  // Follow redirects
        // curlArgs.push_back("-w");  // Write format
        // curlArgs.push_back("URL: %{url_effective}\\n"); 
        curlArgs.push_back("--parallel-max");
        curlArgs.push_back("1");
        curlArgs.push_back("-D");  // Dump headers to stdout
        curlArgs.push_back("-");   // Write headers to stdout
        curlArgs.push_back("--no-progress-meter"); // Suppress progress
        curlArgs.push_back("--retry-delay");
        curlArgs.push_back("2");
        curlArgs.push_back("--retry-max-time");
        curlArgs.push_back("6");
        curlArgs.push_back("-K");
        curlArgs.push_back(configFile.Path().string());
        curlArgs.push_back("--fail-early"); // quit as soon as one transfer fails.
        
        std::string stderrOutput;

        std::string savedHttpHeader;
        
        // Process stdout lines
        auto onStdoutLine = [&](const std::string& line) -> bool {
            if (headersOpt != nullptr)
            {
                headersOpt->push_back(line);
            }
            // Empty line indicates start of new download or end of headers
            if (line.empty())
            {
                if (!savedHttpHeader.empty())
                {
                    std::string lastHeader = savedHttpHeader;
                    savedHttpHeader = "";
                    // Parse status code
                    size_t codeStart = lastHeader.find(' ');
                    if (codeStart != std::string::npos)
                    {
                        codeStart++; // Skip the space
                        size_t codeEnd = lastHeader.find(' ', codeStart);
                        if (codeEnd == std::string::npos)
                        {
                            codeEnd = lastHeader.length();
                        }
                        
                        std::string codeStr = lastHeader.substr(codeStart, codeEnd - codeStart);
                        try
                        {
                            int statusCode = std::stoi(codeStr);
                            
                            // Handle different status codes
                            if (statusCode == 200)
                            {
                                // Successful download
                                currentDownload++;
                                lastStatusCode = 200;
                                if (progressCallback)
                                {
                                    progressCallback(currentDownload, totalDownloads);
                                }
                            }
                            else if (statusCode == 429)
                            {
                                // Retry - curl will handle it, just ignore
                                return true;
                            }
                            else if (statusCode == 503)
                            {
                                // Throttling - save position and expect exit
                                lastStatusCode = 503;
                                Lv2Log::warning("%s","curl: Received 503 response.");
                                return false; 
                            }
                            else if (statusCode >= 300 && statusCode < 400)
                            {
                                // Redirect, &c - advisory only, ignore, and hope curl will deal with it.
                                return true;
                            }
                            else if (statusCode >= 400)
                            {
                                // Error - fatal
                                lastStatusCode = statusCode;
                                return false; // Stop processing
                            }
                        }
                        catch (const std::exception&)
                        {
                            // Failed to parse status code - continue
                            return true;
                        }
                    }
                }
            } else if (line.starts_with("HTTP/"))
            {
                // Delay processing until we get a blank line.
                savedHttpHeader = line;
            }
            // Other lines are headers - ignore unless we need to save them
            return true;
        };
        
        // Execute curl
        int exitCode = curlExec("/usr/bin/curl", curlArgs, onStdoutLine, stderrOutput);

        logHeaders(*headersOpt);

        if (exitCode != EXIT_SUCCESS && exitCode != -1)
        {
            throwCurlExitCode(exitCode);
        }
        
        // Return appropriate status code
        
        return lastStatusCode;
    }

}

