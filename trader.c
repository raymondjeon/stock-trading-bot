#include "trader.h"
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "cJSON.h"



const char OANDA_API_BASE[] = "https://api-fxpractice.oanda.com/v3/";
const char OANDA_API_BASEV1[] = "https://api-fxpractice.oanda.com/v1/";
const char OANDA_PURCHASE_BASE[] = "<account number>/orders";
const char OANDA_INFO_BASE[] = "accounts/101-004-6188772-001/";
const char OANDA_INV_BASE[] = "prices?instruments=";
const char OANDA_HISTORY_BASE[] = "/candles?count=7&price=M&granularity=D";
const char auth_header[] = "Authorization: Bearer <your api key>";

/*
*Appends the second character to the first
*/
static char* append(const char* first, const char* second) {
  const size_t old_length = strlen(first);
  const size_t append_length = strlen(second);
  const size_t new_length = old_length + append_length;
  char* res = (char*) malloc(sizeof(char) * new_length + 1);

  if (res == NULL) {
    perror("append\n");
    exit(EXIT_FAILURE);
  }
  memcpy(res, first, old_length);
  memcpy(res + old_length, second, append_length + 1);
  return res;
}

/*
*A fucntion to be supplied to curl to write the results from
*a https request to the supplied buffer.
*/
size_t write_function(void *ptr, size_t size, size_t nmemb, struct url_data *data) {
    size_t index = data->size;
    size_t n = (size * nmemb);
    char* tmp;

    data->size += (size * nmemb);

    #ifdef DEBUG
    fprintf(stderr, "data at %p size=%ld nmemb=%ld\n", ptr, size, nmemb);
    #endif
    tmp = (char*)realloc(data->data, data->size + 1); /* +1 for '\0' */

    if(tmp) {
        data->data = tmp;
    } else {
        if(data->data) {
            free(data->data);
        }
        fprintf(stderr, "Failed to allocate memory.\n");
        return 0;
    }

    memcpy((data->data + index), ptr, n);
    data->data[data->size] = '\0';

    return size * nmemb;
}

/*
*Set's up the curl easy handle struct with the supplied settings.
*/
static void curl_setup(CURL* handler, char* url, struct url_data* buffer,
  char* post, struct curl_slist* chunk) {
  curl_easy_setopt(handler, CURLOPT_URL, url);
  curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, write_function);
  curl_easy_setopt(handler, CURLOPT_WRITEDATA, buffer);
  curl_easy_setopt(handler, CURLOPT_HTTPHEADER, chunk);
  if (post != NULL) {
    curl_easy_setopt(handler, CURLOPT_POST, 1);
    curl_easy_setopt(handler, CURLOPT_POSTFIELDS, post);
  }
}

/*
*Convert an enum to it's textural representation
*/
static const char* get_inv_from_enum(enum invTypes type) {
  const char* inv;
  switch(type) {
    case EUR_USD: inv = "EUR_USD"; break;
    default: inv = "";
  }
  return inv;
}

/*
*return the easy handle from curl
*/
CURL* init_http_connection() {
  curl_global_init(CURL_GLOBAL_ALL);
  CURL* easyhandle = curl_easy_init();
  if (!easyhandle) {
    perror("error making curl");
    //better to goto a failure case and don't terminate
    exit(EXIT_FAILURE);
  }

  return easyhandle;
}

/*
*get the price data for the given inventory
*/
Inventory *get_inv_info( enum invTypes type) {
  struct curl_slist *chunk = NULL;
  CURL* curl = init_http_connection();
  Inventory *inventory = (Inventory*)malloc(sizeof(Inventory));

  const char* inv = get_inv_from_enum(type);
  char* url = append(OANDA_API_BASEV1, OANDA_INV_BASE);
  url = append(url, inv);
  struct url_data data;
  data.size = 0;
  data.data = (char*)malloc(4096);
  chunk = curl_slist_append(chunk, auth_header);

  curl_setup(curl, url, &data, NULL, chunk);

  CURLcode res = curl_easy_perform(curl);

  /* Check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
  }

  cJSON* root = cJSON_Parse(data.data);
  cJSON *item = cJSON_GetObjectItem(root, "prices");
  cJSON *firstElem = cJSON_GetArrayItem(item, 0);
  cJSON *bidpricejson = cJSON_GetObjectItemCaseSensitive(firstElem, "bid");
  cJSON *askpricejson = cJSON_GetObjectItemCaseSensitive(firstElem, "ask");

  inventory->bid  = bidpricejson->valuedouble;
  inventory->ask = askpricejson->valuedouble;

  close_connection(curl, chunk);
  free(data.data);
  free(url);
  return inventory;
}

/*
*get the net asset value for the given account
*/
double get_account_nav() {
  struct curl_slist *chunk = NULL;
  CURL* curl = init_http_connection();

  char* url = append(OANDA_API_BASE, OANDA_INFO_BASE);
  struct url_data data;
  data.size = 0;
  data.data = (char*)malloc(4096);
  chunk = curl_slist_append(chunk, auth_header);

  curl_setup(curl, url, &data, NULL, chunk);

  CURLcode res = curl_easy_perform(curl);

  /* Check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
  }

  cJSON* root = cJSON_Parse(data.data);
  cJSON *account = cJSON_GetObjectItem(root, "account");
  cJSON *nav = cJSON_GetObjectItemCaseSensitive(account, "NAV");

  double navAmt = atof(nav -> valuestring);

  close_connection(curl, chunk);
  free(data.data);
  free(url);
  return navAmt;
}

/*
*get the last week's performance history for the given inventory
*/
double* get_inv_history( enum invTypes type) {
  struct curl_slist *chunk = NULL;
  CURL* curl = init_http_connection();
  double *pricearray = (double*)malloc(sizeof(double) * 7);

  const char* inv = get_inv_from_enum(type);
  inv = append("instruments/",inv);
  char* url = append(OANDA_API_BASE, inv);
  url = append(url, OANDA_HISTORY_BASE);
  struct url_data data;
  data.size = 0;
  data.data = (char*)malloc(4096);
  chunk = curl_slist_append(chunk, "Content-Type: application/json");
  chunk = curl_slist_append(chunk, auth_header);
  curl_setup(curl, url, &data, NULL, chunk);
  CURLcode res = curl_easy_perform(curl);

  /* Check for errors */
  if(res != CURLE_OK) {
    fprintf(stderr, "curl_easy_perform() failed: %s\n",
      curl_easy_strerror(res));
  }
  cJSON* root = cJSON_Parse(data.data);
  cJSON *item = cJSON_GetObjectItem(root,"candles");
  for(int i = 0; i < 7;i++) {
    cJSON *elem = cJSON_GetArrayItem(item,i);
    cJSON *prices = cJSON_GetObjectItemCaseSensitive(elem, "mid");
    cJSON *openingprice = cJSON_GetObjectItemCaseSensitive(prices, "c");
    pricearray[i] = atof(openingprice->valuestring);
  }

  close_connection(curl, chunk);
  free(data.data);
  free(url);
  return pricearray;
}

/*
*buy the given inventory
*/
enum API_STATUS buy_inv( enum invTypes type, int units) {
  CURL* curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL,
                       "https://api-fxpractice.oanda.com/v3/accounts/101-004-6188772-001/orders");

      struct curl_slist *chunk = NULL;
      char* jsonObj = (char*)"{ \"order\" : {\"units\" : \"100\"  , \"instrument\" : \"EUR_USD\" , \"type\" : \"MARKET\"} }";
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
      chunk = curl_slist_append(chunk, "Content-Type: application/json");
      chunk = curl_slist_append(chunk, "Authorization: Bearer <your api key>");

      struct url_data data;
      data.size = 0;
      data.data = (char*)malloc(4096);

      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);


      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);

      res = curl_easy_perform(curl);

      /* Check for errors */
      if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
      curl_slist_free_all(chunk);
      free(data.data);
    }
  return API_OK;
}

/*
*sell the given inventory
*/
enum API_STATUS sell_inv( enum invTypes type, int units) {
  CURL* curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL,
                       "https://api-fxpractice.oanda.com/v3/accounts/101-004-6188772-001/orders");

      struct curl_slist *chunk = NULL;
      char* jsonObj = (char*)"{ \"order\" : {\"units\" : \"-100\"  , \"instrument\" : \"EUR_USD\" , \"type\" : \"MARKET\"} }";
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
      chunk = curl_slist_append(chunk, "Content-Type: application/json");
      chunk = curl_slist_append(chunk, "Authorization: Bearer <your api key>");
      struct url_data data;
      data.size = 0;
      data.data = (char*)malloc(4096);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


      res = curl_easy_perform(curl);

      /* Check for errors */
      if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
      curl_slist_free_all(chunk);
      free(data.data);
    }
  return API_OK;
}
/*
* closes all open short positions
*/
enum API_STATUS close_short_positions( enum invTypes type) {
  CURL* curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL,
                       "https://api-fxpractice.oanda.com/v3/accounts/101-004-6188772-001/positions/EUR_USD/close");
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      struct curl_slist *chunk = NULL;
      char* jsonObj = (char*)"{\"shortUnits\": \"ALL\" }";
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
      chunk = curl_slist_append(chunk, "Content-Type: application/json");
      chunk = curl_slist_append(chunk, "Authorization: Bearer <your api key>");
      struct url_data data;
      data.size = 0;
      data.data = (char*)malloc(4096);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
      curl_slist_free_all(chunk);
      free(data.data);

    }
  return API_OK;
}

/*
* closes all open long positions
*/
enum API_STATUS close_long_positions( enum invTypes type) {
  CURL* curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
      curl_easy_setopt(curl, CURLOPT_URL,
                       "https://api-fxpractice.oanda.com/v3/accounts/101-004-6188772-001/positions/EUR_USD/close");
      curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
      struct curl_slist *chunk = NULL;
      char* jsonObj = (char*)"{ \"longUnits\": \"ALL\" }";
      curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonObj);
      chunk = curl_slist_append(chunk, "Content-Type: application/json");
      chunk = curl_slist_append(chunk, "Authorization: Bearer <your api key>");
      struct url_data data;
      data.size = 0;
      data.data = (char*)malloc(4096);
      curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
      curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);

      res = curl_easy_setopt(curl, CURLOPT_HTTPHEADER, chunk);


      res = curl_easy_perform(curl);
      /* Check for errors */
      if(res != CURLE_OK) {
          fprintf(stderr, "curl_easy_perform() failed: %s\n",
                  curl_easy_strerror(res));
      }

      curl_easy_cleanup(curl);
      curl_slist_free_all(chunk);
      free(data.data);
    }
  return API_OK;
}


/*
*Perfrom cleanup actions for curl
*/
void close_connection(CURL* handler, struct curl_slist* chunk) {
  curl_easy_cleanup(handler);
  curl_slist_free_all(chunk);
}
