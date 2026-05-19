#pragma once
#include <stdexcept>
#include <string>

// Ierarhie proprie de exceptii

//   catch (const GameException&)               -> orice eroare a jocului
//   catch (const InvalidPlacementException&)   
//   catch (const std::exception&)              -> orice exceptie din proiect
class GameException : public std::runtime_error {
public:
    explicit GameException(const std::string& msg) : std::runtime_error(msg) {}
};


class InsufficientFundsException : public GameException {
public:
    InsufficientFundsException(int needed, int have)
        : GameException("Fonduri insuficiente: e nevoie de " + std::to_string(needed)
                        + ", ai doar " + std::to_string(have)) {}
};


class InvalidPlacementException : public GameException {
public:
    using GameException::GameException;  // mosteneste ctor cu mesaj
};


class IncompatibleEvolutionException : public GameException {
public:
    using GameException::GameException;
};
