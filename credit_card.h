#ifndef CREDIT_CARD_H
#define CREDIT_CARD_H

#include "structures.h"
#include "storage.h"
#include "auth.h"
#include "account.h"
#include <vector>

namespace BankingSystem {

class CreditCardManager {
private:
    Storage& storage;
    AuthManager& authManager;
    AccountManager& accountManager;
    std::vector<CreditCard> cards;

public:
    CreditCardManager(Storage& store, AuthManager& auth, AccountManager& acc);

    void loadCards();
    void saveCards();

    CreditCard* applyForCreditCard(const std::string& customerId, const std::string& bankId, const std::string& linkedAccNo, CreditCardType type);
    bool swipeCard(const std::string& cardNumber, double amount, const std::string& merchant);
    bool payCreditCardBill(const std::string& cardNumber, double amount);

    void printCardStatement(const std::string& cardNumber);

    CreditCard* findCard(const std::string& cardNumber);
    std::vector<CreditCard> getCustomerCards(const std::string& customerId);
};

} // namespace BankingSystem

#endif // CREDIT_CARD_H
