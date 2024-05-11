#pragma once
#include <string>
#include <map>
#include <nanogui/nanogui.h>
#include <fmt/core.h>
#include <iostream>

enum ThreadPosition {
    FIRST,
    SECOND,
    BOTH
};

class Thread {
public:
    int R;
    int G;
    int B;

    Thread(int R, int G, int B) : R(R), G(G), B(B) {};

    nanogui::Color color() {
        return nanogui::Color(nanogui::Vector3i(R, G, B));
    };

    virtual std::string company(ThreadPosition position = ThreadPosition::FIRST) = 0;
    virtual std::string number(ThreadPosition position = ThreadPosition::FIRST) = 0;
    virtual std::string description(ThreadPosition position = ThreadPosition::FIRST) = 0;
    virtual std::string full_name(ThreadPosition position = ThreadPosition::FIRST) = 0;
    virtual bool is_blended() = 0;
    ThreadPosition default_position() {
        return is_blended() ? ThreadPosition::BOTH : ThreadPosition::FIRST;
    }
};

class SingleThread : public Thread {
public:
    SingleThread(std::string company, std::string number,
        std::string description, int R, int G, int B) :
        _company(company), _number(number), _description(description),
        Thread(R, G, B) {};

    virtual std::string company(ThreadPosition position) {
        check_position(position);
        return _company;
    }

    virtual std::string number(ThreadPosition position) {
        check_position(position);
        return _number;
    }

    virtual std::string description(ThreadPosition position) {
        check_position(position);
        return _description;
    }

    virtual std::string full_name(ThreadPosition position) {
        check_position(position);
        return _company + " " + _number;
    }

    virtual bool is_blended() { return false; };

private:
    std::string _company;
    std::string _number;
    std::string _description;

    void check_position(ThreadPosition position) {
        if (position == ThreadPosition::SECOND)
            throw std::invalid_argument("SingleThread does not have a second thread");
    }

    friend class BlendedThread;
};

class BlendedThread : public Thread {
public:
    SingleThread* thread_1;
    SingleThread* thread_2;

    BlendedThread(SingleThread *thread_1, SingleThread *thread_2,
        int R, int G, int B) : thread_1(thread_1), thread_2(thread_2), Thread(R, G, B) {};

    virtual std::string company(ThreadPosition position) {
        switch (position) {
            case ThreadPosition::FIRST: return thread_1->_company;
            case ThreadPosition::SECOND: return thread_2->_company;
            case ThreadPosition::BOTH:
                throw std::invalid_argument("Not implemented");
        }
    }

    virtual std::string number(ThreadPosition position) {
        switch (position) {
            case ThreadPosition::FIRST: return thread_1->_number;
            case ThreadPosition::SECOND: return thread_2->_number;
            case ThreadPosition::BOTH:
                throw std::invalid_argument("Not implemented");
        }
    }

    virtual std::string description(ThreadPosition position) {
        switch (position) {
            case ThreadPosition::FIRST: return thread_1->_description;
            case ThreadPosition::SECOND: return thread_2->_description;
            case ThreadPosition::BOTH:
                return fmt::format("{} mixed with {}", thread_1->_description, thread_2->_description);
        }
    }

    virtual std::string full_name(ThreadPosition position) {
        switch (position) {
            case ThreadPosition::FIRST: return thread_1->full_name(ThreadPosition::FIRST);
            case ThreadPosition::SECOND: return thread_2->full_name(ThreadPosition::FIRST);
            case ThreadPosition::BOTH:
                return fmt::format("{} / {}", thread_1->full_name(ThreadPosition::FIRST), thread_2->full_name(ThreadPosition::FIRST));
        }
    }

    virtual bool is_blended() { return true; };

    bool operator==(const BlendedThread& t) {
        return (thread_1 == t.thread_1 && thread_2 == t.thread_2) ||
               (thread_1 == t.thread_2 && thread_2 == t.thread_1);
    }
};

BlendedThread create_blended_thread(SingleThread *thread_1, SingleThread *thread_2);

class XStitchEditorApplication;

std::string load_manufacturer(const char *file_path, std::map<std::string, Thread*> *map);