#include <stdlib.h>
#include "trader.h"
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <signal.h>

/*
* this calculates the simple moving average over a given length.
*
*/
double calcSimpleMovingAverage(double* prices, double length) {
  int sum = 0;
  for(int i = 0; i < length; i++) {
    sum += prices[i];
  }
  return sum/length;
}

/*
* This calculates the standard deviation from a passsed array of data of a specified length.
*/
float calculateSTD(double data[],double length) {
    float sum = 0;
    float mean = 0;
    float variancesum = 0;
    for(int i = 0; i < length; i++)
    {
        sum += data[i];
    }
    mean = sum/length;
    for(int i = 0; i < length; i++){
        variancesum += pow(data[i] - mean, 2);
    }
    float standarddeviation =  sqrt(variancesum/length);
    return standarddeviation;
}
/*
* checks whether it is the end of the day.
*/
int checkDayEnd(struct tm *localTimeInfo) {
  return ((localTimeInfo->tm_sec <= 59 || localTimeInfo->tm_sec >= 0) && localTimeInfo->tm_min == 59 && localTimeInfo->tm_hour == 23);
}

void printASCIIAndExit(int dummy) {


close_long_positions(EUR_USD);
close_short_positions(EUR_USD);
printf("    _....._\n");
printf("   ';-.--';'\n");
printf("     }===={       _.---.._\n");
printf("   .'  _|_ '.    ';-..--';\n");
printf("  /:: (_|_`  \\    `}===={\n");
printf(" |::  ,_|_)   |  .:  _|_ '.\n");
printf(" \\::.   |     /_;:_ (_|_`  \\\n");
printf("  '::_     _-;'--.-';_|_)   |\n");
printf("      `````  }====={  |     /\n");
printf("           .'  _|_  '.   _.'\n");
printf("          /:: (_|_`   \\``\n");
printf("         |::  ,_|_)    |\n");
printf("         \\::.   |      /\n");
printf("          '::_      _.'\n");
printf("              ``````\n");

printf("******************SESSION EXITED****************\n\n");
printf("An extension by Rishi R, Anthony A, Ashley S, Jordan T\n");
printf("******************SESSION EXITED****************\n\n");

  exit(0);
}

/*
*
* This uses our version of the  Mean Reversion strategy with Bollinger bands to
* to make trading decisions. In addition, this method outputs summaries of trades made to the terminal
* as well as retaining an output log in output.txt.
*/
int main(void) {
  FILE* filePtr = fopen("outputlog.txt", "a");

  int running = 1;
  int dayNumUnitsBought = 0;
  int dayNumUnitsSold = 0;
  float dayMoneyIn = 0;
  float dayMoneyOut = 0;
  int totalNumUnitsHolding = 0;
  double balance;
  int iterations = 0;
  int sixtySecNumUnitsBought = 0;
  int sixtySecNumUnitsSold = 0;
  float sixtySecMoneyIn = 0;
  float sixtySecMoneyOut = 0;

  signal(SIGINT, printASCIIAndExit);
  printf("    _....._\n");
  printf("   ';-.--';'\n");
  printf("     }===={       _.---.._\n");
  printf("   .'  _|_ '.    ';-..--';\n");
  printf("  /:: (_|_`  \\    `}===={\n");
  printf(" |::  ,_|_)   |  .:  _|_ '.\n");
  printf(" \\::.   |     /_;:_ (_|_`  \\\n");
  printf("  '::_     _-;'--.-';_|_)   |\n");
  printf("      `````  }====={  |     /\n");
  printf("           .'  _|_  '.   _.'\n");
  printf("          /:: (_|_`   \\``\n");
  printf("         |::  ,_|_)    |\n");
  printf("         \\::.   |      /\n");
  printf("          '::_      _.'\n");
  printf("              ``````\n");
  printf("Starting automated trading...\n");
  while(running) {
    double* last7dayprices = get_inv_history(EUR_USD);
    double movingaverage = calcSimpleMovingAverage(last7dayprices, 7);
    float doubleSTD = 2*(calculateSTD(last7dayprices,7));
    float upperbollinger = movingaverage + doubleSTD;
    float lowerbollinger = movingaverage - doubleSTD;
    Inventory* currentpricestruct = get_inv_info(EUR_USD);
    double currentprice = currentpricestruct->bid;
    balance = get_account_nav();

    time_t rawTime;
    struct tm *localTimeInfo;
    time(&rawTime);
    localTimeInfo = localtime(&rawTime);


    if (iterations == 2) {
      printf("\n\n*******************************\n\n");
      printf("PROGRESS SUMMARY\n");
      printf("BOUGHT %i UNITS OF EUR_USD\n", sixtySecNumUnitsBought);
      printf("SOLD %i UNITS OF EUR_USD\n", sixtySecNumUnitsSold);
      printf("BALANCE: %f\n", balance);
      printf("\n\n*******************************\n\n");
      sixtySecNumUnitsSold = 0;
      sixtySecNumUnitsBought = 0;
      sixtySecMoneyOut = 0;
      sixtySecMoneyIn = 0;
      iterations = 0;
    }

    if (checkDayEnd(localTimeInfo)) {
      char dailyFileName[sizeof("dd/mm/yyyy") + 1];
      sprintf(dailyFileName, "%02d/%02d/%04dDailySummary.txt", localTimeInfo->tm_mday, localTimeInfo->tm_mon + 1, localTimeInfo->tm_year + 1900);

      FILE* dailyPtr = fopen(dailyFileName, "w");

      fprintf(dailyPtr, "DAILY SUMMARY FOR DATE %i/%i/%i\n\n", localTimeInfo->tm_mday, localTimeInfo->tm_mon + 1, localTimeInfo->tm_year + 1900);
      fprintf(dailyPtr, "TOTAL UNITS BOUGHT TODAY: %i\n", dayNumUnitsBought);
      fprintf(dailyPtr, "TOTAL UNITS SOLD TODAY: %i\n", dayNumUnitsSold);
      fprintf(dailyPtr, "TOTAL MONEY OUT TODAY: %f\n", dayMoneyOut);
      fprintf(dailyPtr, "TOTAL MONEY IN TODAY: %f\n", dayMoneyIn);
      fprintf(dailyPtr, "PROFIT/LOSS: %f\n", dayMoneyIn - dayMoneyOut);
      fprintf(dailyPtr, "TOTAL UNITS CURRENTLY HOLDING: %i\n", totalNumUnitsHolding);
      dayNumUnitsBought = 0;
      dayNumUnitsSold = 0;
      dayMoneyOut = 0;
      dayMoneyIn = 0;
      fputs("\n\n*******************************\n\n", filePtr);
      fprintf(filePtr, "DAILY SUMMARY CREATED IN FILE: %s", dailyFileName);
      fputs("\n\n*******************************\n\n", filePtr);
      fclose(dailyPtr);
    }

    fprintf(filePtr, "TRANSACTION***************************\n");
    fprintf(filePtr, "at TIME/DATE: %s\n", asctime(localTimeInfo));

    if(currentprice < lowerbollinger) {
    // if the current price is lower than the lower bollinger band.
        buy_inv(EUR_USD,100);
        dayNumUnitsBought += 100;
        sixtySecNumUnitsBought += 100;
        totalNumUnitsHolding += 100;

        fputs("BUYING 100 UNITS OF EUR_USD\n", filePtr);
        fprintf(filePtr, "AT PRICE %f PER UNIT\n", currentprice);
        fprintf(filePtr, "FOR TOTAL PRICE OF %f\n", currentprice * 100);
        sixtySecMoneyOut += currentprice * 100;
        dayMoneyOut += currentprice * 100;
        fprintf(filePtr, "CURRENTLY HOLDING %i UNITS OF EUR_USD\n", totalNumUnitsHolding);
        fprintf(filePtr, "BALANCE IS: %f\n", balance);
    } else if(currentprice > upperbollinger) {
      // if the current price is higher than the upper bollinger band.
        sell_inv(EUR_USD,100);
        dayNumUnitsSold += 100;
        sixtySecNumUnitsSold += 100;
        fputs("SELLING 100 UNITS OF EUR_USD", filePtr);
        fprintf(filePtr, "AT PRICE %f PER UNIT\n", currentprice);
        fprintf(filePtr, "FOR TOTAL PRICE OF %f\n", currentprice * 100);
        sixtySecMoneyIn += currentprice * 100;
        dayMoneyIn += currentprice * 100;

        totalNumUnitsHolding -= 100;
        fprintf(filePtr, "CURRENTLY HOLDING %i UNITS OF EUR_USD\n", totalNumUnitsHolding);
        balance += totalNumUnitsHolding * currentprice;
        fprintf(filePtr, "BALANCE IS: %f\n", balance);

    } else {
      printf("NO ACTION taken\n");
    }
    fputs("( •̀_•́)=ε [̲̅$̲̅(̲̅ιοο̲̅)̲̅$̲̅]\n\n", filePtr);
    iterations++;
    sleep(10);
  }
}
