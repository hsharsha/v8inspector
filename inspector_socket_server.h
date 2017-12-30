/*
*    Copyright Node.js contributors. All rights reserved.
*
*    Permission is hereby granted, free of charge, to any person obtaining a copy
*    of this software and associated documentation files (the "Software"), to
*    deal in the Software without restriction, including without limitation the
*    rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
*    sell copies of the Software, and to permit persons to whom the Software is
*    furnished to do so, subject to the following conditions:
*
*    The above copyright notice and this permission notice shall be included in
*    all copies or substantial portions of the Software.
*
*    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
*    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
*    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
*    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
*    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
*    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
*    IN THE SOFTWARE.
*/

#ifndef SRC_INSPECTOR_SOCKET_SERVER_H_
#define SRC_INSPECTOR_SOCKET_SERVER_H_

#include "inspector_agent.h"
#include "inspector_socket.h"
#include "uv.h"

#include <map>
#include <string>
#include <vector>

namespace inspector {

class Closer;
class SocketSession;
class ServerSocket;

class SocketServerDelegate {
 public:
  virtual bool StartSession(int session_id, const std::string& target_id) = 0;
  virtual void EndSession(int session_id) = 0;
  virtual void MessageReceived(int session_id, const std::string& message) = 0;
  virtual std::vector<std::string> GetTargetIds() = 0;
  virtual std::string GetTargetTitle(const std::string& id) = 0;
  virtual std::string GetTargetUrl(const std::string& id) = 0;
  virtual void ServerDone() = 0;
};

// HTTP Server, writes messages requested as TransportActions, and responds
// to HTTP requests and WS upgrades.



class InspectorSocketServer {
 public:
  using ServerCallback = void (*)(InspectorSocketServer*);
  InspectorSocketServer(SocketServerDelegate* delegate,
                        uv_loop_t* loop,
                        const std::string& host,
                        int port,
                        FILE* out = stderr);
  // Start listening on host/port
  bool Start();

  // Called by the TransportAction sent with InspectorIo::Write():
  //   kKill and kStop
  void Stop(ServerCallback callback);
  //   kSendMessage
  void Send(int session_id, const std::string& message);
  //   kKill
  void TerminateConnections();

  int Port() const;

  // Server socket lifecycle. There may be multiple sockets
  void ServerSocketListening(ServerSocket* server_socket);
  void ServerSocketClosed(ServerSocket* server_socket);

  // Session connection lifecycle
  bool HandleGetRequest(InspectorSocket* socket, const std::string& path);
  bool SessionStarted(SocketSession* session, const std::string& id);
  void SessionTerminated(SocketSession* session);
  void MessageReceived(int session_id, const std::string& message) {
    delegate_->MessageReceived(session_id, message);
  }

  int GenerateSessionId() {
    return next_session_id_++;
  }

 private:
  void SendListResponse(InspectorSocket* socket);
  bool TargetExists(const std::string& id);

  enum class ServerState {kNew, kRunning, kStopping, kStopped};
  uv_loop_t* loop_;
  SocketServerDelegate* const delegate_;
  const std::string host_;
  int port_;
  std::string path_;
  std::vector<ServerSocket*> server_sockets_;
  Closer* closer_;
  std::map<int, SocketSession*> connected_sessions_;
  int next_session_id_;
  FILE* out_;
  ServerState state_;

  friend class Closer;
};

}  // namespace inspector

#endif  // SRC_INSPECTOR_SOCKET_SERVER_H_
