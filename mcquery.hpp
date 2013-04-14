#include <boost/asio.hpp>
#include <array>
#include <sstream>

struct mcData {

    bool success = false;
    std::string error;
    std::string motd;
    std::string numplayers;  // int seems better, but this is how we recieve it from minecraft
    std::string maxplayers;
};

struct mcDataSimple : mcData {
    std::string version;
};

struct mcDataBasic : mcData {
    std::string gametype;
    std::string map;
    unsigned short hostport = 0;
    std::string hostip;
};

struct mcDataFull : mcDataBasic {
    std::string& hostname = motd;   // same thing different name
    std::string game_id;
    std::string version;
    std::string plugins;     // only used by bukkit: may need to become a vector of strings
    std::vector<std::array<char,17>> playernames;
};

struct mcQuery {
    mcQuery(const char* host = "localhost",     // move arguments away from constructor?
            const char* port = "25565", 
            const int timeoutsecs = 5);

    mcDataBasic getBasic();
    mcDataFull getFull();
    

private:    // functions
    void challengeReceiver(const boost::system::error_code& error, size_t nBytes);
    void dataReceiver(const boost::system::error_code& error, size_t nBytes);
    void connect();
    void extract(std::istringstream& iss);
    void extractBasic(std::istringstream& iss);
    void extractFull(std::istringstream& iss);

private:    // data
    boost::asio::io_service ioService;
    boost::asio::deadline_timer t;
    boost::asio::ip::udp::resolver Resolver;
    boost::asio::ip::udp::resolver::query Query;
    boost::asio::ip::udp::endpoint Endpoint;
    boost::asio::ip::udp::socket Socket;

    boost::posix_time::time_duration timeout;
    bool fullreq;
    std::array<unsigned char,5000> recvBuffer;   // should look into making the buffer size variable, have to look at boost documentation
    mcDataFull data;
};

struct mcQuerySimple {
    mcQuerySimple(const char* host = "localhost",
            const char* port = "25565", 
            const int timeoutsecs = 5);

    mcDataSimple get();

private:
    void connector(const boost::system::error_code& error);
    void sender(const boost::system::error_code& e, std::size_t numBytes);
    void receiver(const boost::system::error_code& e, std::size_t numBytes);

private:
    boost::asio::io_service ioService;
    boost::asio::deadline_timer t;
    boost::asio::ip::tcp::resolver Resolver;
    boost::asio::ip::tcp::resolver::query Query;
    boost::asio::ip::tcp::endpoint Endpoint;
    boost::asio::ip::tcp::socket Socket;
    boost::posix_time::time_duration timeout;

    std::array<unsigned char,100> recvBuffer;    
    mcDataSimple data;
};
