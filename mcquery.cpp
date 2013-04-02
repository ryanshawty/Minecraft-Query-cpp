// Compiles on my debian system with:
// g++ mcquery.cpp -o mcquery.run -lboost_system -pthread -lboost_thread -std=c++11
#include <boost/asio.hpp>
#include <array>
#include <functional>   // for bind
#include <algorithm>    // std::equal

// just to make stuff readable without polluting the global namespace too much
using endpoint       = boost::asio::ip::udp::endpoint;
using resolver       = boost::asio::ip::udp::resolver;
using udp_socket     = boost::asio::ip::udp::socket;
using boost::asio::deadline_timer;
using boost::asio::io_service;
using boost::asio::buffer;
using boost::posix_time::seconds;
using boost::system::error_code;

using namespace std::placeholders;
using std::exception;
using std::runtime_error;
using std::array;
using std::endl;
using std::cout;
using std::bind;
using std::equal;

using uchar = unsigned char;
using uint  = unsigned int;

/******************
 *  declarations  *
 ******************/
struct mcData {


};

struct mcQuery {
    mcQuery(const char* host = "localhost", 
            const char* port = "25565", 
            const int timeoutsecs = 5);

    mcData get();
    void challengeReceiver(const error_code& error, size_t nBytes);
    void dataReceiver(const error_code& error, size_t nBytes);

private:
    io_service ioService;
    deadline_timer t;
    udp_socket Socket;
    resolver Resolver;
    resolver::query Query;
    endpoint Endpoint;
    array<uchar,100> recvBuffer;
    
    



};

/******************
 *  definitions   *
 ******************/
mcQuery::mcQuery(const char* host /* = "localhost" */, 
                 const char* port /* = "25565" */, 
                 const int timeoutsecs /* = 5 */) 
    : ioService {}, 
      t {ioService, seconds{timeoutsecs}},
      Resolver {ioService},
      Socket {ioService},
      Query {boost::asio::ip::udp::v4(), host, port},
      Endpoint {*Resolver.resolve(Query)}
{ }

mcData mcQuery::get() {
    Socket.connect(Endpoint);
      // request for challenge token
      //             [-magic--]  [type] [----session id------]
    uchar req[] =  { 0xFE, 0xFD, 0x09,  0x01, 0x02, 0x03, 0x04 };
    cout<< "sending..." << endl;
    size_t len = Socket.send_to(buffer(req), Endpoint);  // connectionless UDP: doesn't need to be async
    cout<< "sent " << len << " bytes" << endl;

    t.async_wait(
        [&](const boost::system::error_code& e) {
            if(e) return;
            cout<< "timed out: closing socket";
            Socket.cancel(); // causes event handlers to be called with error code 125 (asio::error::operation_aborted)
        } );
    
    cout<< "preparing recieve buffer" << endl;
    Socket.async_receive_from(buffer(recvBuffer), Endpoint, 
        bind(&mcQuery::challengeReceiver, this, _1, _2));

    try { ioService.run(); } catch(exception& e) {
        cout<< e.what();
    }

    mcData data;
    return data;
}

void mcQuery::challengeReceiver(const error_code& error, size_t nBytes) {
    if(error) return;   // recieve failed, probably cancelled by timer
    cout<< "received " << nBytes << " bytes" << endl;
    // byte 0 is 0x09
    // byte 1 to 4 is the session id (last 4 bytes of the request we sent xor'ed with 0F0F0F0F).
    // These bytes don't hold usefull info, but we check if they are correct anyways
    const array<uchar,5> expected = { 0x09, 0x01, 0x02, 0x03, 0x04 };
    if( !equal(expected.begin(), expected.end(), recvBuffer.begin()) )
        throw runtime_error("Incorrect response from server when recieving data");

    // byte 5 onwards is the challange token: a null-terminated ASCII number string which should be sent back as a 32-bit integer
    uint challtoken = atoi((char*)&recvBuffer[5]);  // unsafe?
    
    // the actual request
    //                      [-magic--]  [type] [----session id------]
    array<uchar,11> req = { 0xFE, 0xFD, 0x00,  0x01, 0x02, 0x03, 0x04 };
    req[10] = challtoken>>0  & 0xFF;
    req[9]  = challtoken>>8  & 0xFF;
    req[8]  = challtoken>>16 & 0xFF;
    req[7]  = challtoken>>24 & 0xFF;
    
    cout<< "sending actual request" << endl;
    Socket.send_to(buffer(req), Endpoint);    
    Socket.async_receive_from(buffer(recvBuffer), Endpoint, bind(&mcQuery::dataReceiver, this, _1, _2));

}

void mcQuery::dataReceiver(const boost::system::error_code& error, size_t nBytes) {
    t.cancel(); // causes event handler to be called with boost::asio::error::operation_aborted
    if(error) return;   // recieve failed
    cout<< "received " << nBytes << " bytes" << endl;
    
    
    
    
    const array<uchar,5> expected = { 0x00, 0x01, 0x02, 0x03, 0x04 };
    if( !equal(expected.begin(), expected.end(), recvBuffer.begin()) )
        throw runtime_error("Incorrect response from server when recieving data");

    for(char e : recvBuffer)
        cout<< e;
}

int main() { 
    mcQuery q;
    q.get();

}
