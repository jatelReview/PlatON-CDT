#include <platon/platon.hpp>
#include <string>
using namespace platon;

class message {
    public:
        std::string head;
        PLATON_SERIALIZE(message, (head))
};

class my_message : public message {
    public:
        std::string body;
        std::string end;
        PLATON_SERIALIZE_DERIVED(my_message, message, (body)(end))
};

extern char const contract_info[] = "info";
extern const char contract_ower[] = "ower";

CONTRACT hello : public platon::Contract{
   public:
      ACTION std::string init(const std::string address = ""){
          if (address.empty()) {
              Address platon_address;
              platon_caller(platon_address);
              contract_ower.self() = platon_address;
          } else {
              contract_ower.self() = Address(address);
          }

          return contract_ower.self().toString();
      }

      ACTION std::vector<my_message> add_message(const my_message &one_message){
          info.self().push_back(one_message);
          return info.self();
      }

      CONST std::vector<my_message> get_message(const std::string &name){
          return info.self();
      }

      ACTION std::string get_ower() {
          Address platon_address = contract_ower.self();
          return platon_address.toString();
      }

      ACTION std::string destroy() {
          Address platon_address;
          platon_origin_caller(platon_address);
          if (contract_ower.self() != platon_address){
              return "invalid address";
          }
          platon_destroy();
          return platon_address.toString();
      }

      ACTION std::string migrate(const bytes &code, uint64_t transfer_value, uint64_t gas_value, 
        const my_message &init_arg){
            Address platon_address;
            platon_origin_caller(platon_address);
            if (contract_ower.self() != platon_address){
                return "invalid address";
            }

            Address return_address;
            platon_migrate(return_address, code, transfer_value, gas_value, init_arg);
            return return_address.toString();
      }

   private:
      StorageType<contract_info, std::vector<my_message>> info;
      StorageType<contract_ower, Address> contract_ower;
};

PLATON_DISPATCH(hello, (init)(add_message)(get_message)(get_ower)(destroy)(migrate))