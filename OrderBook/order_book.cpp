#include <cstdint>
#include <vector>
#include <string>
#include <unordered_map>
#include <map>
#include <deque>
#include <iostream>
#include <iomanip>
#include <chrono>
#include <random>

using namespace std;

struct Order {
    uint64_t orderid;     
    bool isbuy;           
    double price;         
    uint64_t quantity;    
    uint64_t timestampns; 
};

struct PriceLevel {
    double price;
    uint64_t totalquantity; 
};

class OrderBook {
public:
    struct OrderRef {
        bool isbuy;
        double price;
        std::deque<Order>::iterator orderIt;
        OrderRef(bool iby, double pr, std::deque<Order>::iterator oit)
            : isbuy(iby), price(pr), orderIt(oit) {}
        OrderRef() = default; 
    };

    void addorder(const Order& order) {
        if (order.isbuy) {
            bids_[order.price].push_back(order);
            bid_levels_[order.price].price = order.price;
            bid_levels_[order.price].totalquantity += order.quantity;
            order_lookup_[order.orderid] = OrderRef(true, order.price, --bids_[order.price].end());
        } else {
            asks_[order.price].push_back(order);
            ask_levels_[order.price].price = order.price;
            ask_levels_[order.price].totalquantity += order.quantity;
            order_lookup_[order.orderid] = OrderRef(false, order.price, --asks_[order.price].end());
        }
    }

    bool cancelorder(uint64_t orderid) {
        auto it = order_lookup_.find(orderid);
        if (it == order_lookup_.end()) return false;
        bool isbuy = it->second.isbuy;
        double price = it->second.price;
        auto orderIt = it->second.orderIt;
        if (isbuy) {
            auto& dq = bids_[price];
            uint64_t removed_qty = orderIt->quantity;
            bid_levels_[price].totalquantity -= removed_qty;
            dq.erase(orderIt);
            if (dq.empty()) {
                bids_.erase(price);
                bid_levels_.erase(price);
            }
        } else {
            auto& dq = asks_[price];
            uint64_t removed_qty = orderIt->quantity;
            ask_levels_[price].totalquantity -= removed_qty;
            dq.erase(orderIt);
            if (dq.empty()) {
                asks_.erase(price);
                ask_levels_.erase(price);
            }
        }
        order_lookup_.erase(it);
        return true;
    }

    bool amendorder(uint64_t orderid, double newprice, uint64_t newquantity) {
        auto it = order_lookup_.find(orderid);
        if (it == order_lookup_.end()) return false;
        bool isbuy = it->second.isbuy;
        double oldprice = it->second.price;
        auto orderIt = it->second.orderIt;
        if (newprice != oldprice) {
            Order amended = *orderIt;
            amended.price = newprice;
            amended.quantity = newquantity;
            cancelorder(orderid);
            addorder(amended);
        } else {
            uint64_t oldquantity = orderIt->quantity;
            orderIt->quantity = newquantity;
            if (isbuy)
                bid_levels_[oldprice].totalquantity += (newquantity - oldquantity);
            else
                ask_levels_[oldprice].totalquantity += (newquantity - oldquantity);
        }
        return true;
    }

    void get_snapshot(size_t depth, std::vector<PriceLevel>& bids, std::vector<PriceLevel>& asks) const {
        bids.clear();
        asks.clear();

        size_t count = 0;
        for (auto it = bid_levels_.begin(); it != bid_levels_.end() && count < depth; ++it, ++count) {
            bids.push_back(it->second);
        }

        count = 0;
        for (auto it = ask_levels_.begin(); it != ask_levels_.end() && count < depth; ++it, ++count) {
            asks.push_back(it->second);
        }
    }

    void print_book(size_t depth = 10) const {
        std::vector<PriceLevel> bids, asks;
        get_snapshot(depth, bids, asks);

        std::cout << "Order Book Snapshot (Top " << depth << " levels)\n";
        std::cout << "-------------------------------\n";
        std::cout << "   Bids       |      Asks      \n";
        std::cout << "Price  Qty    |  Price   Qty   \n";
        std::cout << "-------------------------------\n";

        for (size_t i = 0; i < depth; ++i) {
            // Print bid
            if (i < bids.size()) {
                std::cout << std::setw(6) << bids[i].price << " ";
                std::cout << std::setw(6) << bids[i].totalquantity << " | ";
            } else {
                std::cout << "              | ";
            }

            // Print ask
            if (i < asks.size()) {
                std::cout << std::setw(6) << asks[i].price << " ";
                std::cout << std::setw(6) << asks[i].totalquantity;
            }
            std::cout << "\n";
        }
        std::cout << "-------------------------------\n";
    }

private: 
    std::map<double, std::deque<Order>, std::greater<double>> bids_;
    std::map<double, std::deque<Order>> asks_;
    std::map<double, PriceLevel, std::greater<double>> bid_levels_;
    std::map<double, PriceLevel> ask_levels_;
    std::unordered_map<uint64_t, OrderRef> order_lookup_; 
};

int main() {
    OrderBook book;
    book.addorder({1, true, 101.5, 10, 100'000});
    book.addorder({2, false, 102.0, 25, 100'005});
    book.addorder({3, true, 101.5, 5, 100'007});
    book.addorder({4, false, 102.0, 15, 100'010});
    book.print_book();

    book.cancelorder(3);  
    book.amendorder(4, 101.6, 30); 
    book.print_book();
    return 0; 
}
