/**
 * SPDX-FileCopyrightText: Copyright (c) 2022-2023, NVIDIA CORPORATION & AFFILIATES.
 * SPDX-License-Identifier: BSD-3-Clause
 */
#pragma once
#include <memory>
#include <utility>

#include <ucp/api/ucp.h>

#include <ucxx/delayed_submission.h>
#include <ucxx/request.h>
#include <ucxx/typedefs.h>

namespace ucxx {

class RequestTag : public Request {
 private:
  size_t _length{0};  ///< The tag message length in bytes

  /**
   * @brief Private constructor of `ucxx::RequestTag`.
   *
   * This is the internal implementation of `ucxx::RequestTag` constructor, made private not
   * to be called directly. This constructor is made private to ensure all UCXX objects
   * are shared pointers and the correct lifetime management of each one.
   *
   * Instead the user should use one of the following:
   *
   * - `ucxx::Endpoint::tagRecv()`
   * - `ucxx::Endpoint::tagSend()`
   * - `ucxx::Worker::tagRecv()`
   * - `ucxx::createRequestTag()`
   *
   * @throws ucxx::Error  if send is `true` and `endpointOrWorker` is not a
   *                      `std::shared_ptr<ucxx::Endpoint>`.
   *
   * @param[in] endpointOrWorker    the parent component, which may either be a
   *                                `std::shared_ptr<Endpoint>` or
   *                                `std::shared_ptr<Worker>`.
   * @param[in] send                whether this is a send (`true`) or receive (`false`)
   *                                tag request.
   * @param[in] buffer              a raw pointer to the data to be transferred.
   * @param[in] length              the size in bytes of the tag message to be transferred.
   * @param[in] tag                 the tag to match.
   * @param[in] enablePythonFuture  whether a python future should be created and
   *                                subsequently notified.
   * @param[in] callbackFunction    user-defined callback function to call upon completion.
   * @param[in] callbackData        user-defined data to pass to the `callbackFunction`.
   */
  RequestTag(std::shared_ptr<Component> endpointOrWorker,
             bool send,
             void* buffer,
             size_t length,
             ucp_tag_t tag,
             const bool enablePythonFuture                               = false,
             std::function<void(std::shared_ptr<void>)> callbackFunction = nullptr,
             std::shared_ptr<void> callbackData                          = nullptr);

 public:
  /**
   * @brief Constructor for `std::shared_ptr<ucxx::RequestTag>`.
   *
   * The constructor for a `std::shared_ptr<ucxx::RequestTag>` object, creating a send or
   * receive tag request, returning a pointer to a request object that can be later awaited
   * and checked for errors. This is a non-blocking operation, and the status of the
   * transfer must be verified from the resulting request object before the data can be
   * released (for a send operation) or consumed (for a receive operation).
   *
   * @throws ucxx::Error  if send is `true` and `endpointOrWorker` is not a
   *                      `std::shared_ptr<ucxx::Endpoint>`.
   *
   * @param[in] endpointOrWorker    the parent component, which may either be a
   *                                `std::shared_ptr<Endpoint>` or
   *                                `std::shared_ptr<Worker>`.
   * @param[in] send                whether this is a send (`true`) or receive (`false`)
   *                                tag request.
   * @param[in] buffer              a raw pointer to the data to be transferred.
   * @param[in] length              the size in bytes of the tag message to be transferred.
   * @param[in] tag                 the tag to match.
   * @param[in] enablePythonFuture  whether a python future should be created and
   *                                subsequently notified.
   * @param[in] callbackFunction    user-defined callback function to call upon completion.
   * @param[in] callbackData        user-defined data to pass to the `callbackFunction`.
   *
   * @returns The `shared_ptr<ucxx::RequestTag>` object
   */
  friend std::shared_ptr<RequestTag> createRequestTag(
    std::shared_ptr<Component> endpointOrWorker,
    bool send,
    void* buffer,
    size_t length,
    ucp_tag_t tag,
    const bool enablePythonFuture,
    std::function<void(std::shared_ptr<void>)> callbackFunction,
    std::shared_ptr<void> callbackData);

  virtual void populateDelayedSubmission();

  /**
   * @brief Create and submit a tag request.
   *
   * This is the method that should be called to actually submit a tag request. It is meant
   * to be called from `populateDelayedSubmission()`, which is decided at the discretion of
   * `std::shared_ptr<ucxx::Worker>`. See `populateDelayedSubmission()` for more details.
   */
  void request();

  /**
   * @brief Callback executed by UCX when a tag send request is completed.
   *
   * Callback executed by UCX when a tag send request is completed, that will dispatch
   * `ucxx::Request::callback()`.
   *
   * WARNING: This is not intended to be called by the user, but it currently needs to be
   * a public method so that UCX may access it. In future changes this will be moved to
   * an internal object and remove this method from the public API.
   *
   * @param[in] request the UCX request pointer.
   * @param[in] status  the completion status of the request.
   * @param[in] arg     the pointer to the `ucxx::Request` object that created the
   *                    transfer, effectively `this` pointer as seen by `request()`.
   */
  static void tagSendCallback(void* request, ucs_status_t status, void* arg);

  /**
   * @brief Callback executed by UCX when a tag receive request is completed.
   *
   * Callback executed by UCX when a tag receive request is completed, that will dispatch
   * `ucxx::RequestTag::callback()`.
   *
   * WARNING: This is not intended to be called by the user, but it currently needs to be
   * a public method so that UCX may access it. In future changes this will be moved to
   * an internal object and remove this method from the public API.
   *
   * @param[in] request the UCX request pointer.
   * @param[in] status  the completion status of the request.
   * @param[in] info    information of the completed transfer provided by UCX, includes
   *                    length of message received used to verify for truncation.
   * @param[in] arg     the pointer to the `ucxx::Request` object that created the
   *                    transfer, effectively `this` pointer as seen by `request()`.
   */
  static void tagRecvCallback(void* request,
                              ucs_status_t status,
                              const ucp_tag_recv_info_t* info,
                              void* arg);

  /**
   * @brief Implementation of the tag receive request callback.
   *
   * Implementation of the tag receive request callback. Verify whether the message was
   * truncated and set that state if necessary, and finally dispatch
   * `ucxx::Request::callback()`.
   *
   * WARNING: This is not intended to be called by the user, but it currently needs to be
   * a public method so that UCX may access it. In future changes this will be moved to
   * an internal object and remove this method from the public API.
   *
   * @param[in] request the UCX request pointer.
   * @param[in] status  the completion status of the request.
   * @param[in] info    information of the completed transfer provided by UCX, includes
   *                    length of message received used to verify for truncation.
   */
  void callback(void* request, ucs_status_t status, const ucp_tag_recv_info_t* info);
};

}  // namespace ucxx
