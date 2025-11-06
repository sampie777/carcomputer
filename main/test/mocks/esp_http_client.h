//
// Created by Samuel-Anton Jansen on 2025/11/06.
//

#ifndef CARCOMPUTER_ESP_HTTP_CLIENT_H
#define CARCOMPUTER_ESP_HTTP_CLIENT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/_types.h>


typedef enum {
    HTTP_METHOD_GET = 0,    /*!< HTTP GET Method */
    HTTP_METHOD_POST,       /*!< HTTP POST Method */
    HTTP_METHOD_PUT,        /*!< HTTP PUT Method */
    HTTP_METHOD_PATCH,      /*!< HTTP PATCH Method */
    HTTP_METHOD_DELETE,     /*!< HTTP DELETE Method */
    HTTP_METHOD_HEAD,       /*!< HTTP HEAD Method */
    HTTP_METHOD_NOTIFY,     /*!< HTTP NOTIFY Method */
    HTTP_METHOD_SUBSCRIBE,  /*!< HTTP SUBSCRIBE Method */
    HTTP_METHOD_UNSUBSCRIBE,/*!< HTTP UNSUBSCRIBE Method */
    HTTP_METHOD_OPTIONS,    /*!< HTTP OPTIONS Method */
    HTTP_METHOD_COPY,       /*!< HTTP COPY Method */
    HTTP_METHOD_MOVE,       /*!< HTTP MOVE Method */
    HTTP_METHOD_LOCK,       /*!< HTTP LOCK Method */
    HTTP_METHOD_UNLOCK,     /*!< HTTP UNLOCK Method */
    HTTP_METHOD_PROPFIND,   /*!< HTTP PROPFIND Method */
    HTTP_METHOD_PROPPATCH,  /*!< HTTP PROPPATCH Method */
    HTTP_METHOD_MKCOL,      /*!< HTTP MKCOL Method */
    HTTP_METHOD_REPORT,     /*!< HTTP REPORT Method */
    HTTP_METHOD_MAX,
} esp_http_client_method_t;

#endif //CARCOMPUTER_ESP_HTTP_CLIENT_H