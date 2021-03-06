/*
 *  Copyright (c) 2014, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/experimental/wangle/ssl/SSLContextConfig.h>
#include <folly/io/async/AsyncServerSocket.h>
#include <folly/io/async/EventBase.h>
#include <proxygen/httpserver/HTTPServerOptions.h>
#include <thread>

namespace proxygen {

class SignalHandler;
class HTTPServerAcceptor;

/**
 * HTTPServer based on proxygen http libraries
 */
class HTTPServer final {
 public:
  /**
   * For each IP you can specify HTTP protocol to use.  You can use plain old
   * HTTP/1.1 protocol or SPDY/3.1 for now.
   */
  enum class Protocol: uint8_t {
    HTTP,
    SPDY,
  };

  struct IPConfig {
    IPConfig(folly::SocketAddress a,
             Protocol p)
        : address(a),
          protocol(p) {}

    folly::SocketAddress address;
    Protocol protocol;
    std::vector<folly::SSLContextConfig> sslConfigs;
  };

  /**
   * Create a new HTTPServer
   */
  explicit HTTPServer(HTTPServerOptions options);
  ~HTTPServer();

  /**
   * Bind server to the following addresses. Can be called from any thread.
   *
   * Throws exception on error (say port is already busy). You can try binding
   * to different set of ports. Though once it succeeds, it is a FATAL to call
   * it again.
   *
   * The list is updated in-place to contain final port server bound to if
   * ephemeral port was given. If the call fails, the list might be partially
   * updated.
   */
  void bind(std::vector<IPConfig>& addrs);

  /**
   * Start HTTPServer.
   *
   * Note this is a blocking call and the current thread will be used to listen
   * for incoming connections. Throws exception if something goes wrong (say
   * somebody else is already listening on that socket).
   *
   * `onSuccess` callback will be invoked from the event loop which shows that
   * all the setup was successfully done.
   *
   * `onError` callback will be invoked if some errors occurs while starting the
   * server instead of throwing exception.
   */
  void start(std::function<void()> onSuccess = nullptr,
             std::function<void(std::exception_ptr)> onError = nullptr);

  /**
   * Stop HTTPServer.
   *
   * Can be called from any thread. Server will stop listening for new
   * connections and will wait for running requests to finish.
   *
   * TODO: Separate method to do hard shutdown?
   */
  void stop();

  /**
   * Get the list of addresses server is listening on. Empty if sockets are not
   * bound yet.
   */
  std::vector<IPConfig> addresses() const {
    return addresses_;
  }

 private:
  HTTPServerOptions options_;

  /**
   * Event base in which we binded server sockets.
   */
  folly::EventBase* mainEventBase_{nullptr};

  /**
   * Server socket
   */
  std::vector<folly::AsyncServerSocket::UniquePtr>
      serverSockets_;

  /**
   * Struct to hold info about handle threads
   */
  struct HandlerThread {
    std::thread thread;

    folly::EventBase* eventBase{nullptr};

    std::vector<std::unique_ptr<HTTPServerAcceptor>> acceptors;

    uint32_t acceptorsRunning{0};
  };

  std::vector<HandlerThread> handlerThreads_;

  /**
   * Optional signal handlers on which we should shutdown server
   */
  std::unique_ptr<SignalHandler> signalHandler_;

  /**
   * Addresses we are listening on
   */
  std::vector<IPConfig> addresses_;
};

}
