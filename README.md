# EOS-DEX
DEX smart contract for EOS blockchain
This contract is a Decentralized Exchange (DEX) smart contract on the EOSIO blockchain. The contract provides three actions:

swap(name from, name to, asset quantity_in, asset quantity_out) - allows users to swap tokens on the exchange. It finds a matching pair of tokens, calculates the output amount, subtracts the input amount from the sender's balance, adds the output amount to the receiver's balance, calculates a fee and transfers it to the fee receiver, updates the reserves, and updates the weights for liquidity provision.

addliquidity(name from, asset quantity0, asset quantity1) - allows users to add liquidity to a token pair on the exchange. It finds a matching pair of tokens, calculates the initial reserves and weights, calculates the liquidity tokens to mint, mints the liquidity tokens to the sender, transfers the input tokens from the sender, and updates the reserves and weights.

removeliquidity(name from, asset liquidity_token) - allows users to remove liquidity from a token pair on the exchange. It finds a matching pair of tokens, transfers the liquidity tokens from the sender, calculates the output token amounts, transfers the output tokens to the sender, and updates the reserves and weights.

The contract uses the EOSIO token contract for the transfer of tokens and the calculation of token amounts. It defines a pair struct to store information about a token pair, including the tokens, reserves, weights, and liquidity token. It also defines several private functions to calculate the output amount, fee, liquidity, and weights for liquidity provision.


Testing


1.Install the EOSIO software development kit (SDK) and the EOSIO contract development toolkit (CDT) on your computer. You can find the installation instructions for your specific operating system on the EOSIO website.
2.Compile your smart contract using the EOSIO CDT

eosio-cpp -abigen -I include -R resource -contract contract_name -o contract_name.wasm contract_name.cpp

This command will generate two files: contract_name.wasm and contract_name.abi. The .wasm file is the compiled binary code of your smart contract, and the .abi file contains the contract's application binary interface (ABI), which defines how other applications can interact with your smart contract.

3.Create a new account on the EOS blockchain and deploy your smart contract to that account using the cleos command-line tool.

cleos set contract account_name /path/to/contract -p account_name@active

4.Test your smart contract by sending transactions to it using the cleos tool. The cleos tool allows you to interact with the EOS blockchain by submitting transactions and querying the blockchain state.

cleos push action account_name method_name '["parameter1", "parameter2"]' -p account_name@active

5.Verify that your smart contract is functioning correctly by checking the blockchain state using the cleos

Verify that your smart contract is functioning correctly by checking the blockchain state using the cleos

