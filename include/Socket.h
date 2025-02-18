#pragma once


class Socket {
public:
    Socket();

    bool createS();

    void closeS() const;

    bool sendData(const char* data) const;

    bool receiveData() const;

    int getS() const;

    void setS(int s);


private:
    int _socketfd;
};
