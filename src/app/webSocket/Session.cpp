#include "Session.h"

void Session::run(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::vector<Users> &users, unsigned int userLimit)
{
    try {
        std::vector<Users>::iterator userIter;
        boost::system::error_code error_code;
        std::stringstream storeIDs;
        beast::flat_buffer buffer;
        size_t bytes = 0;
        buffer.max_size(WEBSOCKET_BUFFER_MAXSIZE);
        buffer.clear();
        
        bytes = websocket->read(buffer, error_code);

        if(bytes == 0) {
            log(debug) << "read 0 byte.";
            websocket->close(websocket::close_code::normal);
            websocket->next_layer().cancel();
            return;
        }

        if(error_code) {
            if (error_code == net::error::eof && error_code == net::error::connection_reset) {
                storeIDs.str("");
                storeIDs.clear();
                for(userIter = users.begin(); userIter != users.end(); userIter++) {
                    if(userIter->websocket_ == websocket) {
                        for(unsigned int j = 0; j < userIter->storeID.size(); j++) {
                            if(j == userIter->storeID.size() - 1) {
                                storeIDs << userIter->storeID[j];
                            } else {
                                storeIDs << userIter->storeID[j] << ", ";
                            }
                        }
                        log(info) << "client disconnected. erase websocket : " << userIter->websocket_ << ", storeID : " << storeIDs.str();
                        users.erase(userIter);
                        websocket->close(websocket::close_code::normal);
                        websocket->next_layer().cancel();
                        break;
                    }
                }
            }
            return;
        }

        Crypt c(WEB_SOCKET_CRYPT_KEY);
        std::stringstream response;        
        std::string temp = net::buffer_cast<const char *>(buffer.data());
        std::string enc_data = temp.substr(0, buffer.size());
        std::string json_data;

        if(enc_data.find("device") == std::string::npos) {
            json_data = c.decrypt_base64(enc_data);
            log(debug) << "encrypt_data    [" << enc_data << "]";
            log(debug) << "decrypt_data    [" << json_data << "]";
        } else {
            json_data = enc_data;
            log(debug) << "decrypt_data    [" << enc_data << "]";
        }

        json::Document json;
        json::ParseResult ok = json.Parse<0>(json_data.c_str());

        if (!ok) {
            log(error) << "json parse error. code:" << ok.Code() << " msg:" << json::GetParseError_En(ok.Code()) << " offset:" << ok.Offset();
            websocket->close(websocket::close_code::normal);
            websocket->next_layer().cancel();
            return;
        }

        json::StringBuffer buf;
        json::Writer<json::StringBuffer> writer(buf);
        json.Accept(writer);
        
        if(json.HasMember("action")) {
            std::string action = "";
            std::string device = "";
            std::string target = "";
            if(json.HasMember("action") && json["action"].IsString()) {
                action = json["action"].GetString();
            }
            if(json.HasMember("device") && json["device"].IsString()) {
                device = json["device"].GetString();
            }
            if(json.HasMember("target") && json["target"].IsString()) {
                target = json["target"].GetString();    
            }

            if(device.compare(DEVICE_WEB) == 0) {              // 매장선택(최초진입)
                if(action.compare(ACTION_ORDER) == 0) { 
                    int is_exist = 0;
                    std::vector<int> vStoreID;
                    vStoreID.clear();

                    // WMPO-3727 다매장 연결
                    //if (json.HasMember("storeID") && json["storeID"].IsArray()) {
                    if (json.HasMember("storeID")) {
                        // 매장번호가 array가 아닐경우 임시 조치
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
                                    websocket->close(websocket::close_code::normal);
                                    websocket->next_layer().cancel();
                                    return;
                                }
                            }
                        } else {
                            // 매장번호가 array가 아닐경우 임시 조치
                            if (json["storeID"].IsInt() && json["storeID"].GetType() == JSON_TYPE_NUMBER) {
                                vStoreID.push_back(json["storeID"].GetInt());
                                storeIDs << json["storeID"].GetInt();
                            } else if (json["storeID"].IsString() && json["storeID"].GetType() == JSON_TYPE_STRING) {
                                vStoreID.push_back(atoi(json["storeID"].GetString()));
                                storeIDs << atoi(json["storeID"].GetString());
                            } else {
                                log(error) << " invalid value storeID type:" << json["storeID"].GetType();
                                websocket->close(websocket::close_code::normal);
                                websocket->next_layer().cancel();
                                return;
                            }
                        }
                    }

                    for(unsigned int i = 0; i < users.size(); i++) {
                        if(users[i].token.compare(json["token"].GetString()) == 0 
                            && users[i].deviceID.compare(json["deviceID"].GetString()) == 0) {
                            users[i].websocket_ = websocket;
                            // WMPO-3727 다매장 연결
                            users[i].token = json["token"].GetString();
                            users[i].deviceID = json["deviceID"].GetString();

                            users[i].storeID = vStoreID;
                            log(info) << "[INIT] WEB TO WEBSOCKET UPDATE CLIENT : [" << websocket
                                      << "], storeID = [" << storeIDs.str()
                                      << "], token = [" << json["token"].GetString()
                                      << "], deviceID = [" << json["deviceID"].GetString() << "]";
                                      
                            is_exist++;
                        }
                    }

                    if(is_exist == 0) {
                        if((unsigned int)users.size() < userLimit) {
                            Users u;
                            u.websocket_ = websocket;
                            // WMPO-3727 다매장 연결
                            u.token = json["token"].GetString();
                            u.deviceID = json["deviceID"].GetString();
                            u.storeID = vStoreID;
                            users.push_back(u);
                        } else {
                            log(error) << "[INIT] CLIENT IS FULL. size : " << users.size();
                            websocket->close(websocket::close_code::normal);
                            websocket->next_layer().cancel();
                            return;
                        }
                        
                        log(info) << "[INIT] WEB TO WEBSOCKET ADD CLIENT : [" << websocket 
                                  << "], storeID = [" << storeIDs.str()
                                  << "], token = [" << json["token"].GetString()
                                  << "], deviceID = [" << json["deviceID"].GetString()  << "]";
                    }
                }
            } else if(device.compare(DEVICE_SERVER) == 0) {     // 주문접수
                if(action.compare(ACTION_ORDER) == 0) { 
                    if(target.compare(DEVICE_WEB) == 0) {
                        json::Document response_json;
                        json::Value &orderInfo = json["orderInfo"];
                        int storeID = 0;
                        response_json.SetObject();

                        json::Document::AllocatorType& allocator = response_json.GetAllocator();
                        if(orderInfo.HasMember("orderNo") && orderInfo["orderNo"].IsString()) {
                            response_json.AddMember("orderNo", orderInfo["orderNo"], allocator);    
                        }
                        if(orderInfo.HasMember("storeID") && orderInfo["storeID"].IsInt()) {
                            storeID = orderInfo["storeID"].GetInt();
                            response_json.AddMember("storeID", orderInfo["storeID"], allocator);    
                        }
                        if(orderInfo.HasMember("storeName") && orderInfo["storeName"].IsString()) {
                            response_json.AddMember("storeName", orderInfo["storeName"], allocator);    
                        }  
                        if(orderInfo.HasMember("orderTime") && orderInfo["orderTime"].IsString()) {
                            response_json.AddMember("orderTime", orderInfo["orderTime"], allocator);    
                        }   
                        if(orderInfo.HasMember("receptionID") && orderInfo["receptionID"].IsString()) {
                            response_json.AddMember("receptionID", orderInfo["receptionID"], allocator);
                        }
                        if(orderInfo.HasMember("receptionNo") && orderInfo["receptionNo"].IsString()) {
                            response_json.AddMember("receptionNo", orderInfo["receptionNo"], allocator);
                        }
                        if(orderInfo.HasMember("receptionType") && orderInfo["receptionType"].IsString()) {
                            int nReceptionType = atoi(orderInfo["receptionType"].GetString());
                            response_json.AddMember("receptionType", nReceptionType, allocator);
                        }
                        if(orderInfo.HasMember("receptionStatus") && orderInfo["receptionStatus"].IsInt()) {
                            response_json.AddMember("receptionStatus", orderInfo["receptionStatus"], allocator);
                        }
                        if(orderInfo.HasMember("receptionStatusText") && orderInfo["receptionStatusText"].IsString()) {
                            response_json.AddMember("receptionStatusText", orderInfo["receptionStatusText"], allocator);
                        }
                        if(orderInfo.HasMember("mobile") && orderInfo["mobile"].IsString()) {
                            response_json.AddMember("mobile", orderInfo["mobile"], allocator);
                        }
                        if(orderInfo.HasMember("address") && orderInfo["address"].IsString()) {
                            response_json.AddMember("address", orderInfo["address"], allocator);
                        }
                        if(orderInfo.HasMember("streetAddress") && orderInfo["streetAddress"].IsString()) {
                            response_json.AddMember("streetAddress", orderInfo["streetAddress"], allocator);
                        }
                        if(orderInfo.HasMember("requirement") && orderInfo["requirement"].IsString()) {
                            response_json.AddMember("requirement", orderInfo["requirement"], allocator);
                        }
                        if(orderInfo.HasMember("payStatusText") && orderInfo["payStatusText"].IsString()) {
                            response_json.AddMember("payStatusText", orderInfo["payStatusText"], allocator);
                        }
                        if(orderInfo.HasMember("totalPaymentAmount") && orderInfo["totalPaymentAmount"].IsInt()) {
                            response_json.AddMember("totalPaymentAmount", orderInfo["totalPaymentAmount"], allocator);
                        }
                        if(orderInfo.HasMember("totalAmount") && orderInfo["totalAmount"].IsInt()) {
                            response_json.AddMember("totalAmount", orderInfo["totalAmount"], allocator);
                        }
                        if(orderInfo.HasMember("totalQty") && orderInfo["totalQty"].IsInt()) {
                            response_json.AddMember("totalQty", orderInfo["totalQty"], allocator);
                        }
                        if(orderInfo.HasMember("totalDiscountAmount") && orderInfo["totalDiscountAmount"].IsInt()) {
                            response_json.AddMember("totalDiscountAmount", orderInfo["totalDiscountAmount"], allocator);
                        }
                        if(orderInfo.HasMember("deliveryPayTypeText") && orderInfo["deliveryPayTypeText"].IsString()) {
                            response_json.AddMember("deliveryPayTypeText", orderInfo["deliveryPayTypeText"], allocator);
                        }
                        if(orderInfo.HasMember("deliveryAmount") && orderInfo["deliveryAmount"].IsInt()) {
                            response_json.AddMember("deliveryAmount", orderInfo["deliveryAmount"], allocator);
                        }
                        if(orderInfo.HasMember("productName") && orderInfo["productName"].IsString()) {
                            response_json.AddMember("productName", orderInfo["productName"], allocator);
                        }

                        json::Value products(json::kObjectType);
                        products = json["orderInfo"]["products"];
                        response_json.AddMember("products", products, allocator);

                        buf.Clear();
                        json::Writer<json::StringBuffer> writer(buf);
                        response_json.Accept(writer);
                        response << buf.GetString();

                        log(debug) << "write_json : " << response.str();

                        // WMPO-3727 다매장 연결
                        for(unsigned int i = 0; i < users.size(); i++) {
                            for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                                if(users[i].storeID[j] == storeID) {
                                    if(send(users[i].websocket_, response)) { 
                                        log(info) << "[ORDER] SERVER to WEB websocket : " << users[i].websocket_ 
                                                  << "], storeID : [" << users[i].storeID[j]
                                                  << "], token : [" << users[i].token 
                                                  << "], deviceID : [" << users[i].deviceID
                                                  << "], message : [" << response.str() << "]";
                                    } else {
                                        // ??
                                    }
                                }
                            }
                        }
                    }
                } else if(action.compare(ACTION_STATUS) == 0) { // 주문상태 변경
                    if(target.compare(DEVICE_WEB) == 0) {
                        int storeID = json["storeID"].GetInt();
                        buf.Clear();
                        json::Writer<json::StringBuffer> writer(buf);
                        json.Accept(writer);
                        response << buf.GetString();

                        // TODO users 컨테이너로 변경
                        // WMPO-3727 다매장 연결
                        for(unsigned int i = 0; i < users.size(); i++) {
                            for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                                if(users[i].storeID[j] == storeID) {
                                    if(send(users[i].websocket_, response)) {
                                        log(info) << "[STATUS] SERVER TO WEB websocket : " << users[i].websocket_ 
                                                  << "], storeID : [" << users[i].storeID[j]
                                                  << "], token : [" << users[i].token 
                                                  << "], deviceID : [" << users[i].deviceID
                                                  << "], message : [" << response.str() << "]";
                                    } else {
                                        // ??
                                    }
                                }
                            }
                        }
                    }
                } else if(action.compare(ACTION_RECEPTION_OFF) == 0) {
                    if(target.compare(DEVICE_WEB) == 0) {
                        int storeID = json["storeID"].GetInt();
                        buf.Clear();
                        json::Writer<json::StringBuffer> writer(buf);
                        json.Accept(writer);
                        response << buf.GetString();

                        // TODO users 컨테이너로 변경
                        // WMPO-3727 다매장 연결
                        for(unsigned int i = 0; i < users.size(); i++) {
                            for(unsigned int j = 0; j < users[i].storeID.size(); j++) {
                                if(users[i].storeID[j] == storeID) {
                                    if(send(users[i].websocket_, response)) {
                                        log(info) << "[RECEPTION_OFF] SERVER TO WEB websocket : " << users[i].websocket_ 
                                                  << "], storeID : [" << users[i].storeID[j]
                                                  << "], token : [" << users[i].token 
                                                  << "], deviceID : [" << users[i].deviceID
                                                  << "], message : [" << response.str() << "]";
                                    } else {
                                        // ??
                                    }
                                }
                            }
                        }
                    }
                }
            }
        } else if (json.HasMember("ping")) {
            json::Document ping_json;
            json::Document::AllocatorType& allocator = ping_json.GetAllocator();
            ping_json.SetObject();
            ping_json.AddMember("ping", "ping_120000", allocator);

            buf.Clear();
            json::Writer<json::StringBuffer> writer(buf);
            ping_json.Accept(writer);
            response << buf.GetString();

            if(users.size() == 0) {
                log(info) << "[PING] not exist connect client";
            } else {
                storeIDs.str("");
                storeIDs.clear();
                for(userIter = users.begin(); userIter != users.end();) {
                    for(unsigned int j = 0; j < userIter->storeID.size(); j++) {
                        if(j == userIter->storeID.size() - 1) {
                            storeIDs << userIter->storeID[j];
                        } else {
                            storeIDs << userIter->storeID[j] << ", ";
                        }
                    }
                    log(info) << "[PING] websocket : [" << userIter->websocket_ 
                                << "], storeID : [" << storeIDs.str()
                                << "], token : [" << userIter->token 
                                << "], deviceID : [" << userIter->deviceID << "]";
                    
                    if(send(userIter->websocket_, response)) {
                        userIter++;
                    } else {
                        log(info) << "[PING] DELETE websocket : [" << userIter->websocket_ 
                                << "], storeID : [" << storeIDs.str()
                                << "], token : [" << userIter->token 
                                << "], deviceID : [" << userIter->deviceID << "]";
                        userIter = users.erase(userIter);
                    }
                }
                log(info) << "[PING] user total count : " << users.size();
            }
        } else {
            log(info) << __FUNCTION__ << "[RUN] action is nothing!";
            websocket->close(websocket::close_code::normal);
            websocket->next_layer().cancel();
            return;
        }
        response.str("");
        response.clear();
        websocket->close(websocket::close_code::normal);
        websocket->next_layer().cancel();
    } catch (const std::exception& e) {
        log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << e.what();
    }    
}

bool Session::send(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::stringstream &message)
{
    boost::system::error_code error_code;
    base64 base;
    std::istringstream in(message.str());
    std::ostringstream out(std::ostringstream::out);
    base.encode(in, out);

    websocket->text(websocket->got_text());
    websocket->write(net::buffer(out.str()), error_code);

    if (error_code) {
        if(error_code != net::error::eof) {
            log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << error_code.message() << ", (" << error_code << ")";
            return false;
        }
    }
    return true;
}

bool Session::send_encrypt(std::shared_ptr<websocket::stream<tcp::socket>> websocket, std::stringstream &message)
{
    boost::system::error_code error_code;
    Crypt c(WEB_SOCKET_CRYPT_KEY);
    const std::string msg = c.encrypt_base64(message.str()) + "\r\n";
    websocket->text(websocket->got_text());
    websocket->write(net::buffer(msg), error_code);

    if (error_code) {
        if(error_code != net::error::eof) {
            log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << error_code.message() << ", (" << error_code << ")";
            return false;
        }
    }
    return true;
}

void Session::ping(std::vector<Users> &users) 
{
    try {
        std::vector<Users>::iterator userIter;        
        std::stringstream response;
        std::stringstream storeIDs;
        json::Document ping_json;

        json::Document::AllocatorType& allocator = ping_json.GetAllocator();
        ping_json.SetObject();
        ping_json.AddMember("ping", "ping_120000", allocator);

        json::StringBuffer buf;
        json::Writer<json::StringBuffer> writer(buf);
        ping_json.Accept(writer);
        response << buf.GetString();

        while(1) {
            if(users.size() == 0) {
                log(info) << "[PING] not exist connect client";
            } else {
                for(userIter = users.begin(); userIter != users.end();) {
                    storeIDs.str("");
                    storeIDs.clear();
                    for(unsigned int j = 0; j < userIter->storeID.size(); j++) {
                        if(j == userIter->storeID.size() - 1) {
                            storeIDs << userIter->storeID[j];
                        } else {
                            storeIDs << userIter->storeID[j] << ", ";
                        }
                    }
                    log(info) << "[PING] websocket : [" << userIter->websocket_ 
                              << "], storeID : [" << storeIDs.str()
                              << "], token : [" << userIter->token 
                              << "], deviceID : [" << userIter->deviceID << "]";
                    
                    if(send(userIter->websocket_, response)) {
                        userIter++;
                    } else {
                        log(info) << "[PING] DELETE websocket : [" << userIter->websocket_ 
                              << "], storeID : [" << storeIDs.str()
                              << "], token : [" << userIter->token 
                              << "], deviceID : [" << userIter->deviceID << "]";
                        userIter = users.erase(userIter);
                    }
                }
                log(info) << "[PING] user total count : " << users.size();
            }
            boost::this_thread::sleep(boost::posix_time::seconds(60));
        }
    } catch (const std::exception& e) {
        log(error) << __FUNCTION__ << " line(" << __LINE__ << ")" << ": " << e.what();
    }
}
