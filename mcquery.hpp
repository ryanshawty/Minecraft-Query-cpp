#include <boost/asio.hpp>
#include <array>
#include <sstream>

struct mcDataBasic {    // needs polymorphic destructor. Do i need to write one myself?
    bool succes = false;
    std::string motd;
    std::string gametype;
    std::string map;
    std::string numplayers;  // int seems better, but this is how we recieve it from minecraft
    std::string maxplayers;
    short hostport = 0;
    std::string hostip;

    virtual void fake() {} // just so it becomes polymorphic
};

struct mcDataFull : mcDataBasic {
    std::string& hostname = motd;   // same thing different name
    std::string game_id;
    std::string version;
    std::string plugins;     // I think only used by bukkit: may need to become a vector of strings
    std::vector<std::array<char,17>> players;
};

struct mcQuery {
    mcQuery(const char* host = "localhost",     // TODO: move arguments away from constructor
            const char* port = "25565", 
            const int timeoutsecs = 5);
    ~mcQuery();

    mcDataBasic getBasic();
    mcDataFull getFull();
    void challengeReceiver(const boost::system::error_code& error, size_t nBytes);      // TODO: move these 2 functions to protected
    void dataReceiver(const boost::system::error_code& error, size_t nBytes);

private:    // functions
    void connect();
    void extract(std::istringstream& iss);
    void extractBasic(std::istringstream& iss);
    void extractFull(std::istringstream& iss);

private:    // data
    boost::asio::io_service ioService;
    boost::asio::deadline_timer t;
    boost::asio::ip::udp::resolver Resolver;
    boost::asio::ip::udp::resolver::query Query;
    boost::asio::ip::udp::socket Socket;
    boost::asio::ip::udp::endpoint Endpoint;

    int timeout;    // TODO: better data type
    bool fullreq;
    std::array<unsigned char,500> recvBuffer;   // should look into making the buffer size variable, have to look in boost documentation
    mcDataBasic* data;
};
