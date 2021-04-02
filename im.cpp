#include<iostream>

#include"im.hpp"

void sql_test()
{
    
   im::TableUser Adm;

//    Adm.Insert("wuyifan","111111");


//    Adm.Insert("liming","111111");
  //  Adm.Insert("lihua","111111");
   // Adm.Delete("zhangsan");
  //  Adm.UpdatePasswd("lihua","222222");
   // Adm.UpdateStatus("liming","online");
  Adm.VerifyUser("wuyifan","111111");
  Json::Value user;
  Adm.SelectAll(&user);
  Json::StyledWriter writer;
  std::cout << writer.write(user) << std::endl;
  }
int main(int argc,char* argv[])
{

    im::IM im_server;
    im_server.Init();
    im_server.Run();

    return 0;
}

