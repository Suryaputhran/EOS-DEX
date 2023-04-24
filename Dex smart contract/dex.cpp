#include <eosio/eosio.hpp>
#include <eosio/asset.hpp>
#include <eosio/system.hpp>
#include <eosio.token/eosio.token.hpp>

using namespace eosio;

class [[eosio::contract("dex")]] dex : public contract {
public:
    using contract::contract;

    [[eosio::action]]
    void swap(name from, name to, asset quantity_in, asset quantity_out) {
        // Find the matching pair
        auto it = std::find_if(pairs.begin(), pairs.end(), [&](const pair& p) {
            return (p.token0 == quantity_in.symbol && p.token1 == quantity_out.symbol) ||
                   (p.token0 == quantity_out.symbol && p.token1 == quantity_in.symbol);
        });
        eosio::check(it != pairs.end(), "Pair not found");

        // Calculate the output amount
        uint64_t input_amount = quantity_in.amount;
        uint64_t input_reserve = (it->token0 == quantity_in.symbol) ? it->reserve0.balance.amount : it->reserve1.balance.amount;
        uint64_t output_reserve = (it->token0 == quantity_in.symbol) ? it->reserve1.balance.amount : it->reserve0.balance.amount;
        uint64_t output_amount = get_output_amount(input_amount, input_reserve, output_reserve);

        // Subtract the input amount from the sender's balance
        eosio::token::transfer_action transfer_act(it->token0.contract, {from, "active"_n});
        transfer_act.send(from, "dex"_n, quantity_in, "swap");

        // Add the output amount to the receiver's balance
        eosio::token::transfer_action transfer_act2(it->token1.contract, {get_self(), "active"_n});
        transfer_act2.send("dex"_n, to, eosio::asset(output_amount, quantity_out.symbol), "");

        // Calculate the fee
        uint64_t fee = calculate_fee(input_amount, *it);
        eosio::token::transfer_action transfer_act3(it->token0.contract, {get_self(), "active"_n});
        transfer_act3.send("dex"_n, it->fee_receiver, eosio::asset(fee, quantity_in.symbol), "fee");

        // Update the reserves
        if (it->token0 == quantity_in.symbol) {
            it->reserve0.balance += quantity_in;
            it->reserve1.balance -= eosio::asset(output_amount, quantity_out.symbol);
        } else {
            it->reserve1.balance += quantity_in;
            it->reserve0.balance -= eosio::asset(output_amount, quantity_out.symbol);
        }

        // Update the weights for liquidity provision
        it->reserve0.weight += input_amount;
        it->reserve1.weight += output_amount;
    }

    [[eosio::action]]
    void addliquidity(name from, asset quantity0, asset quantity1) {
        // Find the matching pair
        auto it = std::find_if(pairs.begin(), pairs.end(), [&](const pair& p) {
            return p.token0 == quantity0.symbol && p.token1 == quantity1.symbol;
        });
        eosio::check(it != pairs.end(), "Pair not found");

                // Calculate the initial reserves and weights
        uint64_t reserve0 = (it->reserve0.balance.symbol == quantity0.symbol) ? it->reserve0.balance.amount : it->reserve1.balance.amount;
        uint64_t reserve1 = (it->reserve0.balance.symbol == quantity0.symbol) ? it->reserve1.balance.amount : it->reserve0.balance.amount;
        uint64_t weight0 = it->reserve0.weight;
        uint64_t weight1 = it->reserve1.weight;

        // Calculate the liquidity tokens to mint
        uint64_t liquidity = calculate_liquidity(quantity0.amount, reserve0, weight0, quantity1.amount, reserve1, weight1);

        // Mint the liquidity tokens to the sender
        eosio::token::transfer_action transfer_act(it->liquidity_token.contract, {get_self(), "active"_n});
        transfer_act.send("dex"_n, from, eosio::asset(liquidity, it->liquidity_token.symbol), "");

        // Transfer the input tokens from the sender
        eosio::token::transfer_action transfer_act2(it->token0.contract, {from, "active"_n});
        transfer_act2.send(from, "dex"_n, quantity0, "add liquidity");

        eosio::token::transfer_action transfer_act3(it->token1.contract, {from, "active"_n});
        transfer_act3.send(from, "dex"_n, quantity1, "add liquidity");

        // Update the reserves and weights
        if (it->reserve0.balance.symbol == quantity0.symbol) {
            it->reserve0.balance += quantity0;
            it->reserve1.balance += quantity1;
            it->reserve0.weight += quantity0.amount;
            it->reserve1.weight += quantity1.amount;
        } else {
            it->reserve1.balance += quantity0;
            it->reserve0.balance += quantity1;
            it->reserve1.weight += quantity0.amount;
            it->reserve0.weight += quantity1.amount;
        }
    }

    [[eosio::action]]
    void removeliquidity(name from, asset liquidity_token) {
        // Find the matching pair
        auto it = std::find_if(pairs.begin(), pairs.end(), [&](const pair& p) {
            return p.liquidity_token.symbol == liquidity_token.symbol;
        });
        eosio::check(it != pairs.end(), "Pair not found");

        // Transfer the liquidity tokens from the sender
        eosio::token::transfer_action transfer_act(it->liquidity_token.contract, {from, "active"_n});
        transfer_act.send(from, "dex"_n, liquidity_token, "remove liquidity");

        // Calculate the liquidity value in the reserves
        uint64_t liquidity = liquidity_token.amount;
        uint64_t reserve0 = it->reserve0.balance.amount;
        uint64_t reserve1 = it->reserve1.balance.amount;
        uint64_t total_supply = it->liquidity_token.supply.amount;
        uint64_t value0 = liquidity * reserve0 / total_supply;
        uint64_t value1 = liquidity * reserve1 / total_supply;

        // Transfer the output tokens to the sender
        eosio::token::transfer_action transfer_act2(it->token0.contract, {get_self(), "active"_n});
        transfer_act2.send("dex"_n, from, eosio::asset(value0, it->token0), "");

        eosio::token::transfer_action transfer_act3(it->token1.contract, {get_self(), "active"_n});
        transfer_act3.send("dex"_n, from, eosio::asset(value1, it->token1), "");

        // Calculate the fee and transfer it
		        uint64_t fee = liquidity * FEE / 10000;
        eosio::token::transfer_action transfer_act4(it->liquidity_token.contract, {get_self(), "active"_n});
        transfer_act4.send("dex"_n, it->fee_collector, eosio::asset(fee, it->liquidity_token.symbol), "");

        // Update the reserves and weights
        if (it->reserve0.balance.symbol == it->token0.symbol) {
            it->reserve0.balance -= eosio::asset(value0, it->token0);
            it->reserve1.balance -= eosio::asset(value1, it->token1);
            it->reserve0.weight -= value0;
            it->reserve1.weight -= value1;
        } else {
            it->reserve1.balance -= eosio::asset(value0, it->token0);
            it->reserve0.balance -= eosio::asset(value1, it->token1);
            it->reserve1.weight -= value0;
            it->reserve0.weight -= value1;
        }

        // Reduce the liquidity supply
        eosio::token::burn_action burn_act(it->liquidity_token.contract, {get_self(), "active"_n});
        burn_act.send(it->liquidity_token, eosio::symbol("DEX", 4), liquidity);

        // Remove the pair if the reserves are empty
        if (it->reserve0.balance.amount == 0 && it->reserve1.balance.amount == 0) {
            pairs.erase(it);
        }
    }

    [[eosio::action]]
    void vote(name voter, symbol_code liquidity_token, bool approve) {
        // Find the matching pair
        auto it = std::find_if(pairs.begin(), pairs.end(), [&](const pair& p) {
            return p.liquidity_token.symbol.code() == liquidity_token;
        });
        eosio::check(it != pairs.end(), "Pair not found");

        // Check if the voter has enough liquidity tokens
        auto balance = eosio::token::get_balance(it->liquidity_token.contract, voter, it->liquidity_token.symbol);
        eosio::check(balance.amount > 0, "Insufficient balance");

        // Update the vote count
        if (approve) {
            it->approved_votes += balance.amount;
        } else {
            it->disapproved_votes += balance.amount;
        }

        // Check if the vote has reached the threshold
        if (it->approved_votes >= VOTE_THRESHOLD) {
            it->fee_collector = it->liquidity_token.contract;
        } else if (it->disapproved_votes >= VOTE_THRESHOLD) {
            it->fee_collector = "eosio.stake"_n;
        }
    }
};


