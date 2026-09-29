#ifndef VALIDATE_CLIENT_HPP
#define VALIDATE_CLIENT_HPP
#include <string>
#include <grpcpp/server_context.h>
namespace
{

/// @result True indicates the client passed a valid access token.
[[nodiscard]] 
bool validateClient(const grpc::CallbackServerContext *context,
                    const std::string &accessToken)
{
    if (accessToken.empty()) { return true; }
    for (const auto &item : context->client_metadata())
    {   
        if (item.first == "x-custom-auth-token")
        {
            if (item.second == accessToken) { return true; }
        }
    }   
    return false;
}

}
#endif
