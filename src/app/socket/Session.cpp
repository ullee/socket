#include "Session.h"

bool Session::send_encrypt(std::shared_ptr<tcp::socket> socket, std::stringstream &message)
{
    // encrypt
    Crypt c(SOCKET_CRYPT_KEY);
    std::string msg = "";
    msg = c.encrypt_base64(message.str());

    log(debug) << "enc : " << msg;

    boost::system::error_code error;
    
    net::write(*socket, net::buffer(msg), error);
    if (error) {
        if(error != net::error::eof) {
            log(error) << "code:" << error.value() << " msg:" << error.message();
            return false;
        }
    }
    return true;
}

bool Session::send_encrypt_logout(std::shared_ptr<tcp::socket> socket, std::stringstream &message, std::vector<Users> &users, std::string token)
{
    // encrypt
    Crypt c(SOCKET_CRYPT_KEY);
    std::string msg = "";


    std::stringstream test;
    test << "{json:111,";

    msg = c.encrypt_base64(test.str());
    //msg = c.encrypt_base64(message.str());


    log(debug) << "enc : " << msg;

    // boost::system::error_code error;
    
    // net::write(*socket, net::buffer(msg), error);
    // if (error) {
    //     if(error != net::error::eof) {
    //         log(error) << "code:" << error.value() << " msg:" << error.message();
    //         return false;
    //     }
    // }
    vector<Users>::iterator user;
    for (user = users.begin(); user != users.end();) {
        if (user->token.compare(token) == 0) {
            log(info) << "erase socket : [" << user->socket_ << "], token : [" << user->token << "]";
            user = users.erase(user);
        } else {
            ++user;
        }
    }

    return true;
}

bool Session::send(std::shared_ptr<tcp::socket> socket, std::stringstream &message)
{
    const std::string msg = message.str();
    boost::system::error_code error;
    net::write(*socket, net::buffer(msg), error);
    if (error) {
        if(error != net::error::eof) {
            log(error) << "code:" << error.value() << " msg:" << error.message();
            return false;
        }
    }
    return true;
}

template<class Archive>
void serialize(Archive &ar, std::vector<int> storeID, const unsigned int version) 
{
    ar & storeID;
}

void Session::run(std::shared_ptr<tcp::socket> socket, std::vector<Users> &users, Instance *xsock)
{
    boost::system::error_code error;

    size_t bytes = 0;
    char ch_buffer[SOCKET_BUFFER_MAXSIZE] = {0,};
    std::string str_buffer = "";

    bytes = net::read(*socket, net::buffer(ch_buffer), net::transfer_at_least(1), error);
    str_buffer.append(ch_buffer);

    size_t mtu_size = 1448;
    // 받은 데이터가 없을경우. L4 healthcheck 등.
    if (bytes == 0) {
        //log(error) << "no data available.";
        //socket->close();
        return;
    } else if ((bytes % mtu_size) == 0) { // MTU 1500 byte. 데이터 유실 방어 처리
        for (unsigned int i = 1; i < (bytes/mtu_size); i++) {
            log(debug) << "lost some packets. read again";
            memset(ch_buffer, 0x00, SOCKET_BUFFER_MAXSIZE);
            bytes = net::read(*socket, net::buffer(ch_buffer), net::transfer_at_least(1), error);
            str_buffer.append(ch_buffer);
            bytes += mtu_size;
            log(debug) << "read again. bytes:" << bytes << " size:" << str_buffer.size() << " [" << str_buffer << "]";
        }
    }

    std::string enc_data = str_buffer.substr(0, bytes);

    if (error) {
        // Client 종료 Signal 처리
        if (error == net::error::eof || error == net::error::connection_reset) {
            log(error) << "client disconnected. code:" << error.value() << " msg:" << error.message();
            socket->close();
            return;
        } else {
            log(error) << "code:" << error.value() << " msg:" << error.message();
        }
    }

    log(debug) << "[REQUEST] origin data. bytes:" << bytes << " size:" << enc_data.size() << " [" << enc_data << "]";
    
    // decrypt
    Crypt c(SOCKET_CRYPT_KEY);
    std::string json_data = c.decrypt_base64(enc_data);

    if (json_data.empty()) {
        log(error) << "request data decrypt error";
        return;
    }

    // read json
    json::Document json;
    json::ParseResult ok = json.Parse<0>(json_data.c_str());

    if (!ok) {
        log(error) << "json parse error. code:" << ok.Code() << " msg:" << json::GetParseError_En(ok.Code()) << " offset:" << ok.Offset();
        return;
    }

    json::StringBuffer buf;
    json::Writer<json::StringBuffer> writer(buf);
    json.Accept(writer);
    log(info) << "[REQUEST] decrypt data. size:" << buf.GetSize() << " [" << buf.GetString() << "]";
    buf.Clear();

    std::stringstream response;

    // WMPO-3727 다매장 연결
    std::stringstream storeIDs;

    if (json.HasMember("action") && json["action"].IsString()) {
        // 최초 연결
        std::string action = json["action"].GetString();
        
        if (action.compare(ACTION_INIT) == 0) {

            // 접속 제한
            if (users.size() > xsock->users_limit) {
                log(error) << "max connection exceed. users count:" << users.size();
                socket->close();
                return;
            }
            
            std::string token = "";
            std::string deviceID = "";
            
            if (json.HasMember("token")) {
                if (json["token"].IsString() && json["token"].GetType() == JSON_TYPE_STRING) {
                    token = json["token"].GetString();
                } else {
                    log(error) << "invalid token value. type:" << json["token"].GetType();
                    return;
                }
            }
                
            if (json.HasMember("deviceID")) {
                if (json["deviceID"].IsString() && json["deviceID"].GetType() == JSON_TYPE_STRING) {
                    deviceID = json["deviceID"].GetString();
                } else {
                    log(error) << "invalid deviceID value. type:" << json["deviceID"].GetType();
                    return;
                }
            }
                
            int existed_count = 0;

            // 기존 소켓이 있으면 업데이트
            for (unsigned int i = 0; i < users.size(); i++) {
                // deviceID + token를 Unique 값으로 판단 한다.
                if (users[i].deviceID.compare(deviceID) == 0 && users[i].token.compare(token) == 0) {
                    
                    log(debug) << "user existed. socket : [" << users[i].socket_
                    << " ], token : [" << users[i].token
                    << " ], deviceID : [" << users[i].deviceID
                    << " ], deviceName : [" << users[i].deviceName << "]";
                    
                    users[i].socket_    = socket;
                    users[i].token      = json["token"].GetString();
                    users[i].deviceID   = json["deviceID"].GetString();
                    users[i].deviceName = json["deviceName"].GetString();
                    
                    existed_count++;

                    log(debug) << "[INIT][UPDATE] socket : [" << users[i].socket_
                    << "], token : [" << users[i].token
                    << "], deviceID : [" << users[i].deviceID
                    << "], deviceName : [" << users[i].deviceName << "]";
                    break;
                }
            }

            // 신규 소켓 생성
            if (existed_count == 0) {
                Users u;
                u.socket_    = socket;
                u.token      = json["token"].GetString();
                u.deviceID   = json["deviceID"].GetString();
                u.deviceName = json["deviceName"].GetString();
                u.storeID.clear();

                users.push_back(u);

                log(debug) << "[INIT] socket : [" << u.socket_
                << "], token : " << u.token
                << "], deviceID:" << u.deviceID
                << "], deviceName:" << u.deviceName << "]";
            }

        // 매장 선택
        } else if (action.compare(ACTION_CHOICE_STORE) == 0) {
            std::vector<int> vStoreID;
            vStoreID.clear();
            storeIDs.str("");
            storeIDs.clear();
            std::string token = "";
            
            // WMPO-3727 다매장 연결
            if (json.HasMember("storeID")) {
                if(json["storeID"].IsArray()) {
                    for(unsigned int i = 0; i < json["storeID"].Size(); i++) {
                        if (json["storeID"][i].IsInt() && json["storeID"][i].GetType() == JSON_TYPE_NUMBER) {
                            vStoreID.push_back(json["storeID"][i].GetInt());
                            if(i == json["storeID"].Size() - 1) {
                                storeIDs << json["storeID"][i].GetInt();
                            } else {
                                storeIDs << json["storeID"][i].GetInt() << ", ";
                            }
                        } else if (json["storeID"][i].IsString() && json["storeID"][i].GetType() == JSON_TYPE_STRING) {
                            vStoreID.push_back(atoi(json["storeID"][i].GetString()));
                            if(i == json["storeID"].Size() - 1) {
                                storeIDs << atoi(json["storeID"][i].GetString());
                            } else {
                                storeIDs << atoi(json["storeID"][i].GetString()) << ", ";
                            }
                        } else {
                            log(error) << " invalid value storeID type:" << json["storeID"][i].GetType();
                            return;
                        }
                    }
                } else {
                    // 매장번호가 배열이 아닌경우 임시조치
                    if (json["storeID"].IsInt() && json["storeID"].GetType() == JSON_TYPE_NUMBER) {
                        vStoreID.push_back(json["storeID"].GetInt());
                        storeIDs << json["storeID"].GetInt();
                    } else if (json["storeID"].IsString() && json["storeID"].GetType() == JSON_TYPE_STRING) {
                        vStoreID.push_back(atoi(json["storeID"].GetString()));
                        storeIDs << atoi(json["storeID"].GetString());
                    } else {
                        log(error) << " invalid value storeID type:" << json["storeID"].GetType();
                        return;
                    }
                }

                if (json.HasMember("token")) {
                    if (json["token"].IsString() && json["token"].GetType() == JSON_TYPE_STRING) {
                        token = json["token"].GetString();
                    } else {
                        log(error) << "invalid token value. type:" << json["token"].GetType();
                        return;
                    }
                }

                log(info) << "[CHOICE_STORE] storeID : [" << storeIDs.str() 
                << "], token : [" << token << "]";

                for (unsigned int i = 0; i < users.size(); i++) {
                    if (users[i].token.compare(token) == 0) {
                        users[i].storeID = vStoreID;

                        // WMPO-3727 다매장 연결 (로그)
                        storeIDs.str("");
                        storeIDs.clear();
                        for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                            if(j == users[i].storeID.size() - 1) {
                                storeIDs << users[i].storeID[j];
                            } else {
                                storeIDs << users[i].storeID[j] << ", ";
                            }
                        }

                        log(debug) << "[CHOSEN] socket : " << users[i].socket_
                        << "], token : [" << users[i].token
                        << "], deviceID : [" << users[i].deviceID
                        << "], deviceName : [" << users[i].deviceName
                        << "], storeID : [" << storeIDs.str() << "]"; 
                    }
                }
            } else {
                // do noting.
            }

        // 신규 주문 알림
        } else if (action.compare(ACTION_ORDER) == 0) {

            int storeID = 0;

            if (json.HasMember("storeID")) {
                if (json["storeID"].IsInt() && json["storeID"].GetType() == JSON_TYPE_NUMBER) {
                    storeID = json["storeID"].GetInt();
                } else if (json["storeID"].IsString() && json["storeID"].GetType() == JSON_TYPE_STRING) {
                    storeID = atoi(json["storeID"].GetString());
                } else {
                    log(error) << "invalid storeID value. type:" << json["storeID"].GetType();
                    return;
                }

                json::StringBuffer buf;
                json::Writer<json::StringBuffer> writer(buf);
                writer.StartObject();
                writer.Key("action");
                writer.String(json["action"].GetString());
                writer.EndObject();
                response << buf.GetString();

                buf.Clear();
                json.RemoveAllMembers();

                // WMPO-3727 다매장 연결
                for (unsigned int i = 0; i < users.size(); i++) {
                    for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                        if (users[i].storeID[j] == storeID) {
                            log(debug) << "[ORDER] socket : [" << users[i].socket_
                            << "], token : [" << users[i].token
                            << "], deviceID : [" << users[i].deviceID
                            << "], deviceName : [" << users[i].deviceName
                            << "], storeID : [" << users[i].storeID[j]
                            << "], message : [" << response.str() << "]";
                            Session::send_encrypt(users[i].socket_, response);
                        }
                    }
                }

            } else {
                // do noting.
            }

        // 주문서 출력
        } else if (action.compare(ACTION_ORDER_PRINT) == 0) {

            std::string token = "";
            
            if (json.HasMember("token")) {
                if (json["token"].IsString() && json["token"].GetType() == JSON_TYPE_STRING) {
                    token = json["token"].GetString();
                } else {
                    log(error) << "invalid token value. type:" << json["token"].GetType();
                    return;
                }
            }

            json::StringBuffer buf;
            json::Writer<json::StringBuffer> writer(buf);
            json.Accept(writer);
            response << buf.GetString();

            buf.Clear();
            json.RemoveAllMembers();
                
            for (unsigned int i = 0; i < users.size(); i++) {
                if (users[i].token.compare(token) == 0) {

                    // WMPO-3727 다매장 연결 (로그)
                    storeIDs.str("");
                    storeIDs.clear();
                    for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                        if(j == users[i].storeID.size() - 1) {
                            storeIDs << users[i].storeID[j];
                        } else {
                            storeIDs << users[i].storeID[j] << ", ";
                        }   
                    }
                    log(debug) << "[ORDER_PRINT] socket : [" << users[i].socket_
                    << "], token : [" << users[i].token
                    << "], deviceID : [" << users[i].deviceID
                    << "], deviceName : [" << users[i].deviceName
                    << "], storeID : [" << storeIDs.str() 
                    << "], message : [" << response.str() << "]";
                    Session::send_encrypt(users[i].socket_, response);
                }
            }

        // 설정 버튼 활성화
        } else if (action.compare(ACTION_ACTIVE) == 0) {

            int storeID = 0;
            json::StringBuffer buf;

            if (json.HasMember("storeID")) {
                if(json["storeID"].IsArray()) {
                    json::Document response_json;
                    json::Value &request_json = json;
                    response_json.SetObject();
                    json::Document::AllocatorType& allocator = response_json.GetAllocator();

                    response_json.AddMember("action", request_json["action"], allocator);
                    if(json.HasMember("companyLoginId")) {
                        response_json.AddMember("companyLoginId", request_json["companyLoginId"], allocator);                   
                    }

                    if(json.HasMember("companyID")) {
                        response_json.AddMember("companyID", request_json["companyID"], allocator);                   
                    }

                    if(json.HasMember("companyName")) {
                        response_json.AddMember("companyName", request_json["companyName"], allocator);                   
                    }

                    buf.Clear();
                    json::Writer<json::StringBuffer> writer(buf);
                    response_json.Accept(writer);
                    response.clear();
                    response.str("");
                    response << buf.GetString();

                    // request storeID serialize
                    std::string reqStoreID = "";
                    std::string userStoreID = "";

                    unsigned int arraySize = json["storeID"].GetArray().Size();
                    for(unsigned int i = 0; i < arraySize; i++) {
                        reqStoreID.append(json["storeID"][i].GetString());
                    } 

                    buf.Clear();
                    json.RemoveAllMembers();
                    
                    // WMPO-3727 다매장 연결
                    for (unsigned int i = 0; i < users.size(); i++) {
                        userStoreID.clear();
                        userStoreID = "";

                        // user storeID serialize
                        for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                            userStoreID.append(to_string(users[i].storeID[j]));
                        }

                        log(debug) << "req storeID : " << reqStoreID << ", user storeID : " << userStoreID << std::endl;

                        if (userStoreID.compare(reqStoreID) == 0) {
                            log(debug) << "[ACTIVE] socket : [" << users[i].socket_
                            << "], token : [" << users[i].token
                            << "], deviceID : [" << users[i].deviceID
                            << "], deviceName : [" << users[i].deviceName
                            << "], message : [" << response.str() << "]";
                            Session::send_encrypt(users[i].socket_, response);
                        }
                    }
                } else {
                    // storeID가 배열이 아닌경우 임시조치
                    if (json["storeID"].IsInt() && json["storeID"].GetType() == JSON_TYPE_NUMBER) {
                        storeID = json["storeID"].GetInt();
                    } else if (json["storeID"].IsString() && json["storeID"].GetType() == JSON_TYPE_STRING) {
                        storeID = atoi(json["storeID"].GetString());
                    } else {
                        log(error) << "invalid storeID value. type:" << json["storeID"].GetType();
                        return;
                    }

                    json::StringBuffer buf;
                    json::Writer<json::StringBuffer> writer(buf);
                    json.Accept(writer);
                    response << buf.GetString();

                    buf.Clear();
                    json.RemoveAllMembers();

                    // WMPO-3727 다매장 연결
                    for (unsigned int i = 0; i < users.size(); i++) {
                        for(unsigned int j = 0; j < users[i].storeID.size(); j++) {                        
                            if (users[i].storeID[j] == storeID) {
                                log(debug) << "[ACTIVE] socket : [" << users[i].socket_
                                << "], token : [" << users[i].token
                                << "], deviceID : [" << users[i].deviceID
                                << "], deviceName : [" << users[i].deviceName
                                << "], storeID : [" << users[i].storeID[j] 
                                << "], message : [" << response.str() << "]";
                                Session::send_encrypt(users[i].socket_, response);
                            }
                        }
                    }
                }
            }
        } else if (action.compare(ACTION_LOGOUT) == 0) {
            std::string token = "";
            std::string deviceID = "";
            if (json.HasMember("token")) {
                if (json["token"].IsString() && json["token"].GetType() == JSON_TYPE_STRING) {
                    token = json["token"].GetString();
                } else {
                    log(error) << "invalid token value. type:" << json["token"].GetType();
                    return;
                }
            }

            if (json.HasMember("deviceID")) {
                if (json["deviceID"].IsString() && json["deviceID"].GetType() == JSON_TYPE_STRING) {
                    deviceID = json["deviceID"].GetString();
                } else {
                    log(error) << "invalid deviceID value. type:" << json["deviceID"].GetType();
                    return;
                }
            }

            json::StringBuffer buf;
            json::Writer<json::StringBuffer> writer(buf);
            writer.StartObject();
            writer.Key("action");
            writer.String(json["action"].GetString());
            writer.EndObject();
            response << buf.GetString();

            buf.Clear();
            json.RemoveAllMembers();

            for (unsigned int i = 0; i < users.size(); i++) {
                if (users[i].token.compare(token) == 0 && users[i].deviceID.compare(deviceID) == 0) {

                    // WMPO-3727 다매장 연결 (로그)
                    storeIDs.str("");
                    storeIDs.clear();
                    for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                        if(j == users[i].storeID.size() - 1) {
                            storeIDs << users[i].storeID[j];
                        } else {
                            storeIDs << users[i].storeID[j] << ", ";
                        }   
                    }
                    log(debug) << "[LOGOUT] socket : [" << users[i].socket_
                    << "], token : [" << users[i].token
                    << "], deviceID : [" << users[i].deviceID
                    << "], deviceName : [" << users[i].deviceName
                    << "], storeID : [" << storeIDs.str() 
                    << "], message : [" << response.str() << "]";
                    Session::send_encrypt_logout(users[i].socket_, response, users, users[i].token);
                }
            }
        }

    // 소켓에 등록된 매장이 있는지 체크
    } else if (json.HasMember("validStoreID")) {

        int storeID = 0;
        
        if (json.HasMember("storeID")) {
            if (json["storeID"].IsInt() && json["storeID"].GetType() == JSON_TYPE_NUMBER) {
                storeID = json["storeID"].GetInt();
            } else if (json["storeID"].IsString() && json["storeID"].GetType() == JSON_TYPE_STRING) {
                storeID = atoi(json["storeID"].GetString());
            } else {
                log(error) << "invalid storeID value. type:" << json["storeID"].GetType();
                socket->close();
                return;
            }

            log(info) << "[VALID_STORE_ID] storeID : [" << storeID << "]";

            std::stringstream response_false, response_true;
            response_false << 0;
            response_true << 1;

            // WMPO-3727 다매장 연결
            for (unsigned int i = 0; i < users.size(); i++) {
                storeIDs.str("");
                storeIDs.clear();
                for(unsigned int j = 0; j < users[i].storeID.size(); j++) {  
                    if(j == users[i].storeID.size() - 1) {
                        storeIDs << users[i].storeID[j];
                    } else {
                        storeIDs << users[i].storeID[j] << ", ";
                    }   
                    
                    if (users[i].storeID[j] == storeID) {
                        Session::send(users[i].socket_, response_true);
                    } else {
                        Session::send(users[i].socket_, response_false);
                    }
                }
                log(debug) << "[VALID_STORE_ID_CHECK] socket : [" << users[i].socket_
                << "], token : [" << users[i].token
                << "], deviceID : [" << users[i].deviceID
                << "], deviceName : [" << users[i].deviceName
                << "], storeID : [" << storeIDs.str() << "]";
            }
        }
    
    // ping thread 가 죽었을 경우 임시 조치용
    } else if (json.HasMember("ping")) {
        
        log(info) << "emergency ping";
        response << "ping_120000"; // 120초

        vector<Users>::iterator user;
        for (user = users.begin(); user != users.end();) {
            storeIDs.str("");
            storeIDs.clear();
            for(unsigned int j = 0; j < user->storeID.size(); j++) {  
                if(j == user->storeID.size() - 1) {
                    storeIDs << user->storeID[j];
                } else {
                    storeIDs << user->storeID[j] << ", ";
                }
            }
            log(debug) << "[PING] socket : [" << user->socket_
            << "], token : [" << user->token
            << "], deviceID : [" << user->deviceID
            << "], deviceName : [" << user->deviceName
            << "], storeID : [" << storeIDs.str() << "]";

            if (Session::send(user->socket_, response) == false) {
                log(info) << "erase socket : [" << user->socket_ << "], token : [" << user->token << "]";
                user = users.erase(user);     
            } else {
                ++user;
            }
        }
        log(info) << "user total count : " << users.size();

    } else {
        // do nothing.
    }

    response.str("");
    response.clear();
}

void Session::ping(std::vector<Users> &users)
{
    std::stringstream response;
    // WMPO-3727 다매장 연결
    std::stringstream storeIDs;
    response << "ping_120000"; // 120초

    for (;;) {
        if (users.size() != 0) {
            
            vector<Users>::iterator user;

            // 모든 클라이언트 ping. 받지못하면 세션 삭제 함.
            for (user = users.begin(); user != users.end();) {
                storeIDs.str("");
                storeIDs.clear();
                for(unsigned int j = 0; j < user->storeID.size(); j++) {  
                    if(j == user->storeID.size() - 1) {
                        storeIDs << user->storeID[j];
                    } else {
                        storeIDs << user->storeID[j] << ", ";
                    }
                }
                log(debug) << "[PING] socket : [" << user->socket_
                << "], token : [" << user->token
                << "], deviceID : [" << user->deviceID
                << "], deviceName : [" << user->deviceName
                << "], storeID : [" << storeIDs.str() << "]";

                if (Session::send(user->socket_, response) == false) {
                    log(info) << "erase socket : [" << user->socket_ << "], token : [" << user->token << "]";
                    user = users.erase(user);
                } else {
                    ++user;
                }
            }
        }
        log(info) << "user total count : " << users.size();
        boost::this_thread::sleep(boost::posix_time::seconds(60));
    }
    response.str("");
    response.clear();
}