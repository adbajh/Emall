/**
 *
 *  LoginFilter.cc
 *
 */

#include "LoginFilter.h"
#include "../database/database.h"

using namespace drogon;

void LoginFilter::doFilter(const HttpRequestPtr &req,
                         FilterCallback &&fcb,
                         FilterChainCallback &&fccb)
{
    //Edit your logic here
    string account = req->getHeader("UserAccount");
    string token = req->getHeader("UserToken");
    // cout << endl;
    // cout << "[Filter]: account=" << account << ", token=" << token << endl;
    if (!token.empty() && !account.empty() && user_get_id(account, token) != -1) {
        //Passed
        fccb();
        return;
    }
    //Check failed
    HttpViewData data;
    auto resp = drogon::HttpResponse::newHttpViewResponse("jump_to_login.csp",data);
    fcb(resp);
}
