#ifndef TRADER_H
#define  TRADER_H

#define INV_LENGTH 7

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <curl/curl.h>


enum invTypes {EUR_USD};
enum API_STATUS {API_OK, API_ERR};

typedef struct Inventory {
  enum invTypes type;
  double bid;
  double ask;
} Inventory;

struct url_data {
    size_t size;
    char* data;
};

typedef struct Inv_history {
  enum invTypes type;
  double price;
  struct Inv_history* next;
} Inv_history;
//get the socket for connection to the api
CURL* init_http_connection();

double* get_inv_history(enum invTypes type);

//get the price info of a inventory
Inventory *get_inv_info( enum invTypes type);

//buy a particular inventory
enum API_STATUS buy_inv( enum invTypes type, int units);

//sell a particular inventory
enum API_STATUS sell_inv( enum invTypes type, int units);

//get the accounts nav
double get_account_nav();
//close the connection and returns 0 if done successfully
void close_connection(CURL*, struct curl_slist*);
//close all open long positions
enum API_STATUS close_long_positions( enum invTypes type);
//close all open short positions
enum API_STATUS close_short_positions( enum invTypes type);


#endif
