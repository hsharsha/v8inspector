#ifndef SRC_INSPECTOR_SOCKET_H_
#define SRC_INSPECTOR_SOCKET_H_

#include "http_parser.h"
#include "uv.h"

#include <string>
#include <vector>

namespace inspector {

enum inspector_handshake_event {
  kInspectorHandshakeUpgrading,
  kInspectorHandshakeUpgraded,
  kInspectorHandshakeHttpGet,
  kInspectorHandshakeFailed
};

class InspectorSocket;

typedef void (*inspector_cb)(InspectorSocket*, int);
// Notifies as handshake is progressing. Returning false as a response to
// kInspectorHandshakeUpgrading or kInspectorHandshakeHttpGet event will abort
// the connection. inspector_write can be used from the callback.
typedef bool (*handshake_cb)(InspectorSocket*,
                             enum inspector_handshake_event state,
                             const std::string& path);

struct http_parsing_state_s {
  http_parser parser;
  http_parser_settings parser_settings;
  handshake_cb callback;
  bool done;
  bool parsing_value;
  std::string ws_key;
  std::string path;
  std::string current_header;
};

struct ws_state_s {
  uv_alloc_cb alloc_cb;
  uv_read_cb read_cb;
  inspector_cb close_cb;
  bool close_sent;
  bool received_close;
};

inline char ToLower(char c) {
  return c >= 'A' && c <= 'Z' ? c + ('a' - 'A') : c;
}

inline bool StringEqualNoCase(const char* a, const char* b) {
  do {
    if (*a == '\0')
      return *b == '\0';
    if (*b == '\0')
      return *a == '\0';
  } while (ToLower(*a++) == ToLower(*b++));
  return false;
}

inline bool StringEqualNoCaseN(const char* a, const char* b, size_t length) {
  for (size_t i = 0; i < length; i++) {
    if (ToLower(a[i]) != ToLower(b[i]))
      return false;
    if (a[i] == '\0')
      return true;
  }
  return true;
}


// The helper is for doing safe downcasts from base types to derived types.
template <typename Inner, typename Outer>
class ContainerOfHelper {
 public:
  inline ContainerOfHelper(Inner Outer::*field, Inner* pointer);
  template <typename TypeName>
  inline operator TypeName*() const;
 private:
  Outer* const pointer_;
};

template <typename Inner, typename Outer>
ContainerOfHelper<Inner, Outer>::ContainerOfHelper(Inner Outer::*field,
                                                   Inner* pointer)
    : pointer_(reinterpret_cast<Outer*>(
          reinterpret_cast<uintptr_t>(pointer) -
          reinterpret_cast<uintptr_t>(&(static_cast<Outer*>(0)->*field)))) {
}

template <typename Inner, typename Outer>
template <typename TypeName>
ContainerOfHelper<Inner, Outer>::operator TypeName*() const {
  return static_cast<TypeName*>(pointer_);
}

template <typename Inner, typename Outer>
inline ContainerOfHelper<Inner, Outer> ContainerOf(Inner Outer::*field,
                                                   Inner* pointer) {
  return ContainerOfHelper<Inner, Outer>(field, pointer);
}


// HTTP Wrapper around a uv_tcp_t
class InspectorSocket {
 public:
  InspectorSocket() : data(nullptr), http_parsing_state(nullptr),
                      ws_state(nullptr), buffer(0), ws_mode(false),
                      shutting_down(false), connection_eof(false) { }
  void reinit();
  void* data;
  struct http_parsing_state_s* http_parsing_state;
  struct ws_state_s* ws_state;
  std::vector<char> buffer;
  uv_tcp_t tcp;
  bool ws_mode;
  bool shutting_down;
  bool connection_eof;
};

int inspector_accept(uv_stream_t* server, InspectorSocket* inspector,
                     handshake_cb callback);

void inspector_close(InspectorSocket* inspector,
                     inspector_cb callback);

// Callbacks will receive stream handles. Use inspector_from_stream to get
// InspectorSocket* from the stream handle.
int inspector_read_start(InspectorSocket* inspector, uv_alloc_cb,
                          uv_read_cb);
void inspector_read_stop(InspectorSocket* inspector);
void inspector_write(InspectorSocket* inspector,
    const char* data, size_t len);
bool inspector_is_active(const InspectorSocket* inspector);

inline InspectorSocket* inspector_from_stream(uv_tcp_t* stream) {
  return ContainerOf(&InspectorSocket::tcp, stream);
}

inline InspectorSocket* inspector_from_stream(uv_stream_t* stream) {
  return inspector_from_stream(reinterpret_cast<uv_tcp_t*>(stream));
}

inline InspectorSocket* inspector_from_stream(uv_handle_t* stream) {
  return inspector_from_stream(reinterpret_cast<uv_tcp_t*>(stream));
}

}  // namespace inspector


#endif  // SRC_INSPECTOR_SOCKET_H_
