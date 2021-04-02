#include<cstdio>
#include<iostream>
#include<sstream>
#include<mysql/mysql.h>
#include<mutex>
#include<jsoncpp/json/json.h>
#include"mongoose.h"
#include<list>
using namespace std;

namespace im
{
#define MYSQL_HOST "127.0.0.1"
#define MYSQL_USER "root"
#define MYSQL_PASSWD "818817Ma.."
#define MYSQL_DB "im_system"
#define OFFLINE "offline"
#define ONLINE "online"
   class TableUser
  {
    public:
      TableUser()
        // 构造，完成数据库operator init；
        :_mysql(NULL)
        {
            _mysql = mysql_init(NULL);
            if(_mysql == nullptr)
            {
                std::cout << "初始化数据库句柄失败" << std::endl;
                exit(-1);
            }
            if(mysql_real_connect(_mysql,MYSQL_HOST,MYSQL_USER,MYSQL_PASSWD,MYSQL_DB,0,NULL, 0) == NULL)
            {
                std::cout << "连接数据库失败" << std::endl;
                mysql_close(_mysql);
                exit(-1);
            }
            if(mysql_set_character_set(_mysql,"utf8")!= 0)
            {
                std::cout << " 设置字符集失败:" << mysql_error(_mysql) << std::endl;
                mysql_close(_mysql);
                exit(-1);
            }
         //这里其实还有一个选择数据库，但是其实在建立连接的时候已经制定了，所以可以不完成此处功能
        } 
      ~TableUser()
       {
         //析构不再使用的资源，销毁数据库句柄
         if(_mysql)
         {
             mysql_close(_mysql);
         }
       }
       //完成数据库的基本成员函数，做外接接口：增，删，改，查；
       bool Insert(const string& name,const string& passwd)//(id, name, passwd, status)
       {
#define INSET_USER "insert tb_user value(null,'%s',MD5('%s'),'%s');"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,INSET_USER, name.c_str(),passwd.c_str(),OFFLINE);
           return Query_Sql(temp_sql);
       }
       bool Delete(const string& name)//根据用户名来进行删除；
       {
#define DELETE_USER "delete from tb_user where name = '%s';"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,DELETE_USER,name.c_str());
           return Query_Sql(temp_sql);

       }

       bool UpdateStatus(const string& name,const string& status)
       {
#define UPDATE_USER_statu "update tb_user set status = '%s' where name = '%s';"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,UPDATE_USER_statu,status.c_str(),name.c_str());
           return Query_Sql(temp_sql);
       }
       bool UpdatePasswd(const string& name, const string& passwd)
       {
#define UPDATE_USER_PASSWD "update tb_user set passwd =MD5( '%s') where name = '%s';"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,UPDATE_USER_PASSWD,passwd.c_str(),name.c_str());
           return Query_Sql(temp_sql);
       }

       bool SelectOne(const string& name, Json::Value* user)//通过使用json类型的数据类型存储数据
       {
#define SELECT_USER_ONE "select id,passwd,status from tb_user where name = '%s';"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,SELECT_USER_ONE,name.c_str());
           _mutex.lock();
           if(Query_Sql(temp_sql) == false)
           {
               _mutex.unlock();
               return false;
           }//插入成功后，需要查询时，需要保存结果集
        MYSQL_RES* res = mysql_store_result(_mysql);
        _mutex.unlock();
        if(res == NULL)
        {
            std::cout << "保存单条结果集失败：" << mysql_error(_mysql) << std::endl;
            return false;
        }
        //结果集保存成功后，根据数据库查询方式，需要获取行，列
        int num_row = mysql_num_rows(res);
        if(num_row != 1)//这里使用where name 查询，是一条数据，保证安全性
        {
            std::cout << " 数据条数有问题" << std::endl;
            mysql_free_result(res);
            return false;
        }
        for(int i = 0; i < num_row; i++)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            (*user)["id"] = stoi(row[0]);
            (*user)["name"] = name.c_str();
            (*user)["passwd"] = row[1];
            (*user)["status"] = row[2];
        }
       }
       bool SelectAll(Json::Value* users)
       {
#define SELECT_USER_ALL "select* from tb_user;"
           _mutex.lock();
           if(Query_Sql(SELECT_USER_ALL) == false)
           {
               _mutex.unlock();
               return false;
           }//插入成功后，需要查询时，需要保存结果集
        MYSQL_RES* res = mysql_store_result(_mysql);
        _mutex.unlock();
        if(res == NULL)
        {
            std::cout << "保存所有结果集失败：" << mysql_error(_mysql) << std::endl;
            return false;
        }
        //结果集保存成功后，根据数据库查询方式，需要获取行，列
        int num_row = mysql_num_rows(res);

        for(int i = 0; i < num_row; i++)
        {
            MYSQL_ROW row = mysql_fetch_row(res);
            Json::Value user;
            user["id"] = stoi(row[0]);
            user["name"] = row[1];
            user["passwd"] = row[2];
            user["status"] = row[3];
            users->append(user);
       }
        mysql_free_result(res);
        return true;
       }
       bool VerifyUser(const string& name,const string& passwd)//根据用户名密码确认是否正确；
       {
#define VERIFY_USER "select* from tb_user where name = '%s' and passwd =MD5( '%s');"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,VERIFY_USER,name.c_str(),passwd.c_str());
           _mutex.lock();
           if(Query_Sql(temp_sql)== false)
           {
               std::cout << "确认失败" << endl;
               _mutex.unlock();
               return false;
           }
         MYSQL_RES* res = mysql_store_result(_mysql);
         _mutex.unlock();
         if(res == NULL)
         {
             std::cout << "确认阶段保存结果集失败：" << mysql_error(_mysql) << std::endl;
         }
         //获取行列
         int num_row = mysql_num_rows(res);
         if(num_row != 1)
         {
             std::cout << "验证失败" << endl;
             mysql_free_result(res);
             return false;
         }
         mysql_free_result(res);
         std::cout <<"验证成功" << std::endl;
         return true;

       }
       bool Exists(const string& name)//通过用户名查询数据库内是否存在该用户
       {
#define EXISTS_USER "select* from tb_user where name = '%s';"
           char temp_sql[4096] = {0};
           sprintf(temp_sql,EXISTS_USER,name.c_str());
           _mutex.lock();
           if(Query_Sql(temp_sql)== false)
           {
               std::cout << "exists 考核失败" << endl;
               _mutex.unlock();
               return false;
           }
         MYSQL_RES* res = mysql_store_result(_mysql);
         _mutex.unlock();
         if(res == NULL)
         {
             std::cout << "存在确认阶段保存结果集失败：" << mysql_error(_mysql) << std::endl;
         }
         //获取行列
         int num_row = mysql_num_rows(res);
         if(num_row != 1)
         {
             std::cout << "存在验证失败" << endl;
             mysql_free_result(res);
             return false;
	   
         }
         std::cout << "Exists OK" << std::endl;
       }

    private:
       bool Query_Sql(const string& sql)
       {
           if(mysql_query(_mysql,sql.c_str())!= 0)
           {
               std::cout << "query sql:" << sql.c_str() << "失败：" << mysql_error(_mysql) << endl;
               return false;
           }
           return true;
       } //这里将插入语句封装，提高代码复用率；
    private:
      MYSQL* _mysql;
      mutex _mutex;//锁的操作句柄，为后续查询，保存提供锁操作

};
    struct session
{
    uint64_t session_id;
    string name;
    string statu;//状态
    double login_time;
    double last_atime;//
    struct mg_connection* conn;
};
class IM
{
    public:
    ~IM()
    {
        mg_mgr_free(&_mgr);//释放句柄
    }
   static bool Init(const string& port = "9000")
    {
        _tb_user = new TableUser();//建立数据库操作对象，交互api
        mg_mgr_init(&_mgr);
        string addr = "0.0.0.0:";
        addr += port;
        _lst_http = mg_http_listen(&_mgr,addr.c_str(),callback,&_mgr);
        if(_lst_http == NULL)
        {
            cout << "http listen failed!" << endl;
            return false;
        }
        cout << "http listen true!" << endl;
        return true;
    }
    static bool Run()
    {
        while(1)
        {
            mg_mgr_poll(&_mgr,1000);
        }
        return true;

    }

    private:
        static int Split(const string& str, const string& sep,vector<string>* vec)
        {
            int count = 0;
            size_t pos = 0,idx = 0;
            while(1)
            {
                pos = str.find(sep,idx);//从str字符串的idx位置找sep分隔符
                if(pos == string::npos)
                {
                    break;
                }
                vec->push_back(str.substr(idx,pos - idx));
                idx = pos + sep.size();
                count++;
                if(idx < str.size())
                {
                    vec->push_back(str.substr(idx));
                    count++;
                }
            }
            return count;
        }
        static bool GetCookie(const string& cookie,const string& key,string* val)
        {
            //根据cookie格式，根据格式分隔部分信息，
            vector<string> vec;
            int count = Split(cookie,";",&vec);
            for(auto e:vec)
            {
                vector<string> arry_cookie;

                Split(e,"=",&arry_cookie);
                if(arry_cookie[0] == key)
                {
                    *val = arry_cookie[1];
                    return true;
                }
            }
            
            return false;
        }
       static void CreatSession(struct session* s,struct mg_connection* c,const string& name)
        {
            s->name = name;
            s->session_id = (uint64_t)(mg_time()*100000);//mongoose库里的，单位是微秒
            s->login_time = mg_time();
            s->last_atime = mg_time();
            return;
        }
        static void DelSession(struct mg_connection* c)
        {
            auto it = _list.begin();
            for(;it != _list.end();it++)
            {
                if(it->conn == c)
                {
                    cout << "delete session:" << it->name << endl;
                    _list.erase(it);
                    return;
                }
            }
            return;
        }
        static struct session* GetSessionByName(const string& name)
        {
            auto it = _list.begin();
            for(;it != _list.end();it++)
            {
                if(it->name == name)
                {
                    return&(*it);
                }
            }
            return NULL;
        }
         static struct session* GetSessionByConn(struct mg_connection* c)
        {
            
            auto it = _list.begin();
            for(;it != _list.end();it++)
            {
                if(it->conn == c)
                {
                    return &(*it);
                }
           }
            return NULL;
        }
         
        static bool reg(struct mg_connection* c,struct mg_http_message*hm)
        {
           //正文-》提交的用户信息  json格式字符串；
           int status = 200;

           string header = "Content-Type:application/json\r\n";
           string body;
           body.assign(hm->body.ptr,hm->body.len);
           Json::Value user;
           Json::Reader reader;
           bool ret = reader.parse(body,user);
           if(ret == false)
           {
               status == 400;
               mg_http_reply(c,status,header.c_str(),"{\"reson\":\"请求格式错误\"}");
           }   //解析user ，passwd；
        
           //判断是否被占用
           ret = _tb_user->Exists(user["name"].asString());
           if(ret == true)
           {
               status = 400;
               mg_http_reply(c,status,header.c_str(),"{\"reson\":\"用户名被占用\"}");
               return false;
           }
           //将用户信息插入数据库中； 
           ret = _tb_user->Insert(user["name"].asString(),user["passwd"].asString());
           if(ret == false)
           {
               status == 400;
               mg_http_reply(c,status,header.c_str(),"{\"reason\":\"数据库请求错误\"}");
               return false;
           }
            mg_http_reply(c,status,header.c_str(),"{\"reason\":\"注册成功1\"}");
            return true;
        }
        static bool login(struct mg_connection* c,struct mg_http_message* hm)
        {
            int rsp_status = 200;
           string rsp_body = "{\"reson\":\"注册成功\"}";
            string rsp_header = "Content-Type:application/json\r\n";
           string req_body;
          req_body.assign(hm->body.ptr,hm->body.len);
          Json::Value user;
          Json::Reader reader;
         bool ret = reader.parse(req_body,user);
        if(ret == false)
        {
            rsp_status = 400;
            rsp_body = "{\"reason\":\"请求格式错误\"}";
            mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
            return false;
        } 
            ret = _tb_user->VerifyUser(user["name"].asString(),user["passwd"].asString());
            if(ret == false)
            {
                rsp_status = 403;
                rsp_body = "{\"reason\":\"用户名或密码错误\"}";
                mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
                return false;
            }
           ret =  _tb_user->UpdateStatus(user["name"].asString(),ONLINE);
           if(ret == false)
           {
               rsp_status = 500;
               rsp_body = "{\"reason\":\"修改用户状态出错\"}";
               mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
               return false;
           }
            struct session s;
            CreatSession(&s,c,user["name"].asString());
            _list.push_back(s);
            stringstream cookie;
            cookie << "Set-Cookie:SESSIO_ID=" << s.session_id << "; path = /\r\n";
            cookie << "Set-Cookie:NAME="<< s.name << "; path = /\r\n";
            rsp_header += cookie.str();
            mg_http_reply(c,rsp_status,rsp_header.c_str(),rsp_body.c_str());
            return true;
        }
        static void Broadcast(const string& msg)
        {
            struct mg_connection* c;
            for(c = _mgr.conns; c != NULL;c = c->next)
            {
                if(c-> is_websocket)
                {
                    mg_ws_send(c,msg.c_str(),msg.size(),WEBSOCKET_OP_TEXT);
                }
            }
            return;
        }
        static void callback(struct mg_connection* c,int ev,void* ev_data,void* fn_data)
        {
            struct mg_http_message* hm = (struct mg_http_message*)ev_data;
            struct mg_ws_message* wm = (struct mg_ws_message*)ev_data;
            switch(ev)
            {
                case MG_EV_HTTP_MSG:
                    {
                        if(mg_http_match_uri(hm,"/reg"))
                        {
                            //注册表单数据；
                            reg(c,hm);
                        }else if(mg_http_match_uri(hm,"/login"))
                        {
                            //登录的表单数据请求
                            login(c,hm);
                        }else if(mg_http_match_uri(hm,"/websocket")){
                            //websocketd的握手请求
                            //检测是否是已经登录状态；
                            struct mg_str* cookie_str = mg_http_get_header(hm,"Cookie");
                            if(cookie_str == NULL)
                            {
                                //未登录用户，没有cookie
                                string body = R"({"reason":"未登录"})";
                                mg_http_reply(c,403,"Content-Type: application/json\r\n",body.c_str());    
                                return;
                            }
                            string name;
                            string temp_cook;
                            temp_cook.assign(cookie_str->ptr,cookie_str->len);
                            GetCookie(temp_cook,"NAME",&name);
                           // cout << name << endl;
                            string msg = name + "加入聊天室。。大家欢迎欢迎。";
                            Broadcast(msg);
                            mg_ws_upgrade(c,hm,NULL);
                        }else{
                            //静态页面请求
                            //这里需要检测一下cookie信息），如果没有检测到session的话，跳转到登录页面
                            if(hm->uri.ptr != "./login.html")
                            {
                                //获取cookie，根据name，查询session，没有则没登录，
                                
                           // struct mg_str* cookie_str = mg_http_get_header(hm,"Cookie");
                           // if(cookie_str == NULL)
                           // {
                                //未登录用户，没有cookie
                             //   string body = R"({"reason":"未登录"})";
                              //  mg_http_reply(c,403,"Content-Type: application/json\r\n",body.c_str());    
                                //这里直接跳；
                           // }

                                
                            }
                            struct mg_http_serve_opts opts = {.root_dir = "./web_root"};
                            mg_http_serve_dir(c,hm,&opts);
                        }
                        break;
                    }
                case MG_EV_WS_MSG:
                    {
                        string msg;
                        msg.assign(wm->data.ptr,wm->data.len);
                        Broadcast(msg);
                    }
                    break;
                case MG_EV_CLOSE:
                    {
                    struct session* ss = GetSessionByConn(c);
                    if(ss != NULL)
                    {
                        string msg = ss->name + "退出聊天室。。。。";
                        Broadcast(msg);
                        _tb_user->UpdateStatus(ss->name,OFFLINE);
                        DelSession(c);

                    }
                    break;
                    }
                default:
                    break;
            }
            return;
        }
    private:
        static TableUser* _tb_user;
        static struct mg_mgr _mgr;
        static struct mg_connection* _lst_http;
        string addr;
       // string port = "9000";
       static list<struct session>_list;//使用链表实现session，因为需要频繁的进行插入删除，这样性能比较好

};
    TableUser* IM::_tb_user = NULL;
    struct mg_connection* IM::_lst_http = NULL;    
    struct mg_mgr IM::_mgr;
    list<struct session>IM::_list;
}

