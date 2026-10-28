#include "credit_card.h"
#include "utils.h"
#include <iostream>
#include <iomanip>

namespace BankingSystem
{

    CreditCardManager::CreditCardManager(Storage &store, AuthManager &auth, AccountManager &acc)
        : storage(store), authManager(auth), accountManager(acc)
    {
        loadCards();
    }

    void CreditCardManager::loadCards()
    {
        cards = storage.loadCreditCards();
    }

    void CreditCardManager::saveCards()
    {
        storage.saveCreditCards(cards);
    }

    CreditCard *CreditCardManager::applyForCreditCard(const std::string &customerId, const std::string &bankId, const std::string &linkedAccNo, CreditCardType type)
    {
        Customer *cust = authManager.findCustomer(customerId);
        if (!cust)
            return nullptr;

        Money limit = Money::fromRupees(50000.0);
        int minScore = 650;
        switch (type)
        {
        case CreditCardType::CLASSIC:
            limit = Money::fromRupees(50000.0);
            minScore = 650;
            break;
        case CreditCardType::GOLD:
            limit = Money::fromRupees(150000.0);
            minScore = 700;
            break;
        case CreditCardType::PLATINUM:
            limit = Money::fromRupees(300000.0);
            minScore = 750;
            break;
        case CreditCardType::SUPER_PREMIUM:
            limit = Money::fromRupees(1000000.0);
            minScore = 800;
            break;
        }

        if (cust->creditScore < minScore)
        {
            Utils::printError("Credit Card Application Rejected! Credit score (" + std::to_string(cust->creditScore) + ") below required minimum (" + std::to_string(minScore) + ") for " + creditCardTypeToString(type));
            return nullptr;
        }

        CreditCard cc;
        cc.cardNumber = Utils::generateRandomId("4532");
        cc.customerId = customerId;
        cc.bankId = bankId;
        cc.linkedAccountNo = linkedAccNo;
        cc.type = type;
        cc.creditLimit = limit;
        cc.availableLimit = limit;
        cc.currentOutstanding = Money{};
        cc.rewardPoints = 100.0; // Bonus welcome points
        cc.cvv = 341;
        cc.expiryDate = "12/2030";
        cc.isBlocked = false;

        cards.push_back(cc);
        saveCards();

        authManager.logAction("CREDIT_CARD_ISSUED", bankId, "Issued " + creditCardTypeToString(type) + " Card #" + cc.cardNumber + " with Limit " + Utils::formatCurrency(static_cast<double>(limit)));
        Utils::printSuccess("Credit Card Approved! Card Number: " + cc.cardNumber + " | Credit Limit: " + Utils::formatCurrency(static_cast<double>(limit)));
        return &cards.back();
    }

    bool CreditCardManager::swipeCard(const std::string &cardNumber, double amount, const std::string &merchant)
    {
        const Money swipeAmount = Money::fromRupees(amount);
        CreditCard *cc = findCard(cardNumber);
        if (!cc || cc->isBlocked)
        {
            Utils::printError("Card not found or blocked.");
            return false;
        }

        if (swipeAmount > cc->availableLimit)
        {
            Utils::printError("Transaction Declined: Over credit limit.");
            return false;
        }

        cc->availableLimit -= swipeAmount;
        cc->currentOutstanding += swipeAmount;
        cc->rewardPoints += (amount / 100.0) * 2.0; // 2 points per ₹100

        saveCards();
        authManager.logAction("CARD_SWIPE", cc->bankId, "Swiped " + Utils::formatCurrency(amount) + " at " + merchant);
        Utils::printSuccess("Transaction Approved at " + merchant + "! Amount: " + Utils::formatCurrency(amount) + " | Available Limit: " + Utils::formatCurrency(static_cast<double>(cc->availableLimit)));
        return true;
    }

    bool CreditCardManager::payCreditCardBill(const std::string &cardNumber, double amount)
    {
        const Money paymentAmount = Money::fromRupees(amount);
        CreditCard *cc = findCard(cardNumber);
        if (!cc || cc->currentOutstanding <= Money{})
        {
            Utils::printError("Card not found or no outstanding bill.");
            return false;
        }

        Account *acc = accountManager.findAccount(cc->linkedAccountNo);
        if (!acc || acc->balance < paymentAmount)
        {
            Utils::printError("Insufficient account balance to pay credit card bill.");
            return false;
        }

        acc->balance -= paymentAmount;
        cc->currentOutstanding -= paymentAmount;
        cc->availableLimit += paymentAmount;

        saveCards();
        accountManager.saveAccounts();
        Utils::printSuccess("Credit Card Bill Payment Received: " + Utils::formatCurrency(amount) + ". Remaining Bill: " + Utils::formatCurrency(static_cast<double>(cc->currentOutstanding)));
        return true;
    }

    void CreditCardManager::printCardStatement(const std::string &cardNumber)
    {
        CreditCard *cc = findCard(cardNumber);
        if (!cc)
        {
            Utils::printError("Credit Card not found.");
            return;
        }

        Utils::printHeader("CREDIT CARD STATEMENT — CARD #" + cc->cardNumber);
        std::cout << " Card Type        : " << creditCardTypeToString(cc->type) << "\n";
        std::cout << " Total Limit      : " << Utils::formatCurrency(static_cast<double>(cc->creditLimit)) << "\n";
        std::cout << " Available Limit  : " << Utils::formatCurrency(static_cast<double>(cc->availableLimit)) << "\n";
        std::cout << " Total Outstanding: " << Utils::formatCurrency(static_cast<double>(cc->currentOutstanding)) << "\n";
        std::cout << " Min Amount Due   : " << Utils::formatCurrency(static_cast<double>(cc->currentOutstanding) * 0.05) << " (5%)\n";
        std::cout << " Reward Points    : " << cc->rewardPoints << " pts\n";
        std::cout << " Expiry           : " << cc->expiryDate << "\n";
        Utils::printDivider('=', 70);
    }

    CreditCard *CreditCardManager::findCard(const std::string &cardNumber)
    {
        for (auto &c : cards)
        {
            if (c.cardNumber == cardNumber)
                return &c;
        }
        return nullptr;
    }

    std::vector<CreditCard> CreditCardManager::getCustomerCards(const std::string &customerId)
    {
        std::vector<CreditCard> res;
        for (const auto &c : cards)
        {
            if (c.customerId == customerId)
                res.push_back(c);
        }
        return res;
    }

} // namespace BankingSystem
