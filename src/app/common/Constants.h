#define SOCKET                      "SOCKET"
#define WEB_SOCKET                  "WEB_SOCKET"

#define SOCKET_CRYPT_KEY            1
#define WEB_SOCKET_CRYPT_KEY        2

#define DEVICE_POS                  "POS"
#define DEVICE_APP                  "APP"
#define DEVICE_WEB                  "WEB"
#define DEVICE_SERVER               "SERVER"
    
#define DATA_TYPE_LIST              "LIST"
    
#define ACTION_INIT                 "INIT"
#define ACTION_ORDER                "ORDER"
#define ACTION_ACTIVE               "ACTIVE"
#define ACTION_ORDER_PRINT          "PRINT"
#define ACTION_STATUS               "STATUS"
#define ACTION_CHOICE_STORE         "CHOICE_STORE"
#define ACTION_RECEPTION_OFF        "RECEPTION_OFF"
#define ACTION_LOGOUT               "LOGOUT"

#define RECEPTION_READY             10
#define RECEPTION_MAKING            20
#define RECEPTION_SUCCESS           30
#define RECEPTION_REJECT            40

#define ENCRYPT_AES256_CBC          "AES-256-CBC"

#define REASON_SOLD_OUT             "죄송합니다. 아래 주문 메뉴 또는 옵션이 품절되어 주문이 취소되었습니다."
#define REASON_DELAY                "죄송합니다. 제조지연 사유로 주문이 취소되었습니다."
#define REASON_ETC                  "죄송합니다. 기타 사유로 주문이 취소되었습니다."

#define JSON_TYPE_NULL              0
#define JSON_TYPE_FALSE             1
#define JSON_TYPE_TRUE              2
#define JSON_TYPE_OBJECT            3
#define JSON_TYPE_ARRAY             4
#define JSON_TYPE_STRING            5
#define JSON_TYPE_NUMBER            6