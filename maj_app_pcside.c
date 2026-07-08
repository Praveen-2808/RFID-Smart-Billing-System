/*
 * maj_app_pcside.c
 * -----------------------------------------------------------------------
 * PC-side (Linux) companion application for the "Smart Cart" RFID
 * shopping-cart system. Communicates with the microcontroller board
 * over a USB-serial link (see uart.c/uart.h) and maintains the shop's
 * data in plain CSV files:
 *   stock.csv  - item catalogue: name,card,qty,price
 *   bank.csv   - bank accounts:  card,pin,balance
 *   income.csv - log of completed sale amounts with timestamps
 *
 * Message protocol received from the microcontroller (see
 * major_app_ucside.c for the sending side):
 *   "<manager_card>"        - manager card scanned -> open manager menu
 *   "D<card>$"               - delete one unit of item with this card from cart
 *   "B<card>A<amt>$"          - verify a bank card exists (checkout: card payment)
 *   "P<pin>A<amt>$"           - verify PIN and attempt to deduct amt from balance
 *   "S<amt>$" / "C$"          - transaction finished: Sale completed / Cancelled
 *   "exit"                    - shut down the PC application
 *   anything else             - treated as an item card scan (add to cart)
 * -----------------------------------------------------------------------
 */
#include<stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <string.h>
#include<unistd.h>
#include "uart.h"
#include<errno.h>
#include<stdio_ext.h>
#include<time.h>
//#define TEST_MODE

/* Card number that identifies the shop manager and opens the manager
 * menu (stock entry / price update / quantity update) instead of being
 * treated as a customer item scan. */
char manager_card[10]="12608170";
/* In-memory representation of one line item currently in the active
 * shopping cart. */
struct cart
{
	char name[50];   /* item name */
	char card[10];   /* item's RFID card number (used as its unique ID) */
	int qty;         /* remaining stock quantity after taking this unit */
	int price;       /* unit price */
	int count;       /* how many units of this item are currently in the cart */
};
/* Dynamically-sized array of cart line items (grown with realloc as
 * items are scanned); cart_count tracks how many entries are in use. */
struct cart *cart_items=NULL;
/* Details of the bank card most recently looked up by verify_Bankcard(),
 * used afterwards by verify_pin(). */
struct bank
{
	char card[10];
	int pin;
	int bal;
}bank_details;
int cart_count=0;   /* number of distinct items currently in cart_items */
int total;          /* running total of all completed sales (for income.csv "TOTAL" line) */
int fd;             /* serial port file descriptor */
FILE *fp;            /* general-purpose file handle reused across functions */

/* Sends a null-terminated string to the microcontroller over the
 * already-open serial port, one byte at a time. */
void uart_send(char *p)
{
	printf("send\n");
	while(*p)
	{
		serialPutchar (fd,*p);
		++p;
	}
}
/* Returns 0 if every character in the string is a decimal digit,
 * or 1 if any non-digit character is found (used to validate numeric
 * input like card numbers, quantities and prices). */
int checkint(char *p)
{
	while(*p)
	{
		if(!(*p>='0'&&*p<='9'))
			return 1;
		++p;
	}
	return 0;
}
/* Opens `filename` in the given mode and reads its entire contents into
 * a newly-allocated, null-terminated buffer, which is returned. The
 * file is left open (via the global `fp`) and rewound to the start so
 * the caller can also write back to it afterwards. Exits the program if
 * the file cannot be opened. Note: `file_buff` parameter is unused as
 * input (only its allocated replacement is returned) - kept as-is. */
void *openfile(char *filename,char *mode,char *file_buff)
{
	fp=fopen(filename,mode);
	if(fp==NULL)
	{
		perror("fopen");
		//return 0;
		exit(0);
	}
	/* Determine file size by seeking to the end, then rewind. */
	fseek(fp,0,2);
	int size=ftell(fp)+1;
	rewind(fp);
	/* Allocate a buffer for the whole file (+1 byte handled via size),
	 * read it in, rewind again for any subsequent write, and
	 * null-terminate the buffer so it can be treated as a C string. */
	file_buff=calloc(size,1);
	fread(file_buff,size-1,1,fp);
	rewind(fp);
	file_buff[size-1]='\0';
	return file_buff;
}
/* Interactive manager menu (used when the manager card is scanned):
 *   1. Entry        - append a new item (name,card,qty,price) to stock.csv
 *   2. Update price - find an item by card number and edit its price in-place
 *   3. Update Qty   - find an item by card number and edit its quantity in-place
 *   4. Back         - return to the main receive loop
 * Menu options 2 and 3 use the classic "read whole file into memory,
 * locate & resize the digits in place, write back" approach so that
 * price/quantity values of a different digit-length can be substituted
 * without disturbing the rest of the CSV file. */
void manager_fun()
{
	char item_name[50],item_card[10];
	char *file_buff=NULL;
	char *p=NULL,*q=NULL,*k=NULL;
	int size,total_len,l1,l2,posn,i;
	int qty,price;
	char c;
	int flag=0;
	while(1)
	{
		__fpurge(stdin);
		puts("------MENU------");
		printf("1.Entry\n2.Update price\n3.Update Quantity\n4.Back\n");
		printf("select choice:");
		scanf("%c",&c);
		switch(c)
		{
			case '1':fp=fopen("stock.csv","a");
				if(fp==NULL)
				{
					perror("fopen\n");
					//return 0;
					exit(0);
				}
				__fpurge(stdin);
				printf("enter item name:");
				fgets(item_name,50,stdin);
				/* Strip the trailing newline left by fgets(). */
				item_name[strlen(item_name)-1]='\0';
				card1:printf("Enter the Card Number:");
				__fpurge(stdin);
				fgets(item_card,10,stdin);
				item_card[strlen(item_card)-1]='\0';
				/* Card numbers must be exactly 8 numeric digits. */
				if((strlen(item_card)!=8)||checkint(item_card))
				{
					printf("Enter 8 digits Card number\n");
					goto card1;
				}
				qty1:printf("Enter the Quantity:");
				__fpurge(stdin);
				scanf("%d",&qty);
				if(qty<0)
				{
					printf("Enter Quantity Again\n");
					goto qty1;
				}
				price1:printf("Enter the item price:");
				__fpurge(stdin);
				scanf("%d",&price);
				if(price<0)
				{
					printf("Enter Price Again\n");
					goto price1;
				}
				/* Append the new item as a CSV row. */
				fprintf(fp,"%s,%s,%d,%d\n",item_name,item_card,qty,price);
				fclose(fp);
				break;
			case '2':
				/* Load the whole stock file into memory so the price
				 * field can be located and resized in place. */
				file_buff=openfile("stock.csv","r+",file_buff);
				//char card_buf[10];
				__fpurge(stdin);
				card2:printf("Enter card Number to Update price\n");
				fgets(item_card,10,stdin);
				item_card[strlen(item_card)-1]='\0';
				if((strlen(item_card)!=8)||checkint(item_card))
				{
					printf("Enter 8 digits Card number\n");
					goto card2;
				}
				/* Locate the row containing this card number. */
				p=strstr(file_buff,item_card);
				if(p==NULL)
				{
					printf("Item not found\n");
					exit(0);
					//return;
				}
				/* Skip past "<card>," (8 digits + comma = 9 chars) to
				 * the start of the quantity field, then past the
				 * quantity field's comma to the start of the price
				 * field. */
				p=p+9;
				q=p;
				while(*q!=',')
				{
					++q;
				}
				++q;
				char price[11];
				__fpurge(stdin);
				price2:printf("Enter New Price:");
				fgets(price,11,stdin);
				price[strlen(price)-1]='\0';
				if((strlen(price)==0)||checkint(price))
				{
					printf("Enter price Again\n");
					goto price2;
				}
				/* Measure how long the existing price field is (up to
				 * the newline) vs. the new price string, then grow or
				 * shrink the buffer and shift the remaining file
				 * content accordingly so the new price fits exactly. */
				total_len=0;l1=0;l2=0;
				k=q;
				while(*k!='\n')
				{
					++l1;
					++k;
				}
				l2=strlen(price);
				total_len=strlen(file_buff);
				posn=q-file_buff;
				if(l2>l1)
				{
					/* New price is longer: grow the buffer and shift
					 * everything after the price field forward. */
					file_buff=realloc(file_buff,total_len+(l2-l1)+1);
					k=file_buff+posn;
					memmove(k+(l2-l1),k,strlen(k)+1);
				}
				else if(l2<l1)
				{
					/* New price is shorter: shift everything after the
					 * price field backward, then shrink the buffer. */
					k=file_buff+posn;
					memmove(k,k+(l1-l2),strlen(k+(l1-l2))+1);
					file_buff=realloc(file_buff,total_len-(l1-l2)+1);
				}
				/* Copy the new price digits into the now-correctly-sized gap. */
				q=file_buff+posn;
				i=0;
				while(price[i]!='\0')
				{
					*q=price[i++];
					++q;
				}
				/* Write the updated buffer back over the whole file. */
				fprintf(fp,"%s",file_buff);
				fclose(fp);
				//printf("%s\n",file_buff);
				break;
			case '3':
				/* Same in-place edit approach as case '2', but for the
				 * quantity field instead of price. */
				file_buff=openfile("stock.csv","r+",file_buff);
                                //char card_buf[10];
                                __fpurge(stdin);
				card3:printf("Enter card Number to Update Quantity\n");
                                fgets(item_card,10,stdin);
                                item_card[strlen(item_card)-1]='\0';
				if((strlen(item_card)!=8)||checkint(item_card))
				{
					printf("Enter 8 digits Card number\n");
					goto card3;
				}
                                /* Locate the row containing this card number. */
                                p=strstr(file_buff,item_card);
                                if(p==NULL)
                                {
                                        printf("Item not found\n");
                                        exit(0);
                                        //return;
                                }
                                /* Skip past "<card>," to the start of the quantity field. */
                                p=p+9;
                                //char *q=p;
                                char qty[11];
                                __fpurge(stdin);
				qty2:printf("Enter New Quantity:");
                                fgets(qty,11,stdin);
                                qty[strlen(qty)-1]='\0';
				if((strlen(qty)==0)||checkint(qty))
				{
					printf("Enter quantity Again\n");
					goto qty2;
				}
                                total_len,l1=0,l2=0;
                                q=p;
                                /* Measure the existing quantity field's length (up to its comma). */
                                while(*q!=',')
                                {
                                        ++l1;
                                        ++q;
                                }
                                l2=strlen(qty);
                                total_len=strlen(file_buff);
                                posn=p-file_buff;
                                if(l2>l1)
                                {
                                        /* New quantity is longer: grow the buffer and shift the
                                         * remaining content forward. */
                                        file_buff=realloc(file_buff,total_len+(l2-l1)+1);
                                        q=file_buff+posn;
                                        memmove(q+(l2-l1),q,strlen(q)+1);
                                }
                                else if(l2<l1)
                                {
                                        /* New quantity is shorter: shift the remaining content
                                         * backward, then shrink the buffer. */
                                        q=file_buff+posn;
                                        memmove(q,q+(l1-l2),strlen(q+(l1-l2))+1);
                                        file_buff=realloc(file_buff,total_len-(l1-l2)+1);
                                }
                                /* Copy the new quantity digits into the now-correctly-sized gap. */
                                q=file_buff+posn;
                                i=0;
                                while(qty[i]!='\0')
                                {
                                        *q=qty[i++];
                                        ++q;
                                }
                                /* Write the updated buffer back over the whole file. */
                                fprintf(fp,"%s",file_buff);
                                fclose(fp);
                                //printf("%s\n",file_buff);
                                break;
			case '4':return;
		}
	}
}
/* Handles a customer scanning an item's RFID card to add it to the cart:
 *   - If this card is already in cart_items, decrement its remaining
 *     qty and increment its in-cart count.
 *   - Otherwise, look it up in stock.csv, add a new cart_items entry,
 *     and decrement its qty.
 * Then updates the stock.csv qty field in place (same buffer-splice
 * technique as manager_fun()), forwards the item's name/price/remaining
 * qty back to the microcontroller ("#name,price,qty$"), and prints the
 * current cart contents to the console. */
void item_card(char *card)
{
	char line[100],qty_buf[12],tx_buf[80];
	char *p=NULL;
	int flag=0,qty_len=0,posn;
	char *file_buff=NULL;
	file_buff=openfile("stock.csv","r+",file_buff);
	rewind(fp);
	while(fgets(line,sizeof(line),fp))
	{
		if(strstr(line,card))
		{
			/* Check whether this item is already present in the cart. */
			for(int i=0;i<cart_count;i++)
			{
				if(strcmp(cart_items[i].card,card)==0)
				{
				/* Already in cart: re-read its latest stock line (in
				 * case another process/session changed it), then
				 * decrement its quantity and bump its in-cart count. */
				sscanf(line,"%[^,],%[^,],%d,%d",cart_items[i].name,cart_items[i].card,&cart_items[i].qty,&cart_items[i].price);
				qty_len=snprintf(NULL,0,"%d",cart_items[i].qty);
				--cart_items[i].qty;
				sprintf(qty_buf,"%d",cart_items[i].qty);
				++cart_items[i].count;
				sprintf(tx_buf,"#%s,%d,%d$",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
				flag=1;
				break;
				}
			}
		if(flag==0)
		{
			/* Not yet in cart: allocate a new cart_items slot for it,
			 * parse its stock line, and decrement its quantity. */
			++cart_count;
			cart_items=realloc(cart_items,cart_count*sizeof(struct cart));
			sscanf(line,"%[^,],%[^,],%d,%d",cart_items[cart_count-1].name,cart_items[cart_count-1].card,&cart_items[cart_count-1].qty,&cart_items[cart_count-1].price);
			qty_len=snprintf(NULL,0,"%d",cart_items[cart_count-1].qty);
			--cart_items[cart_count-1].qty;
			sprintf(qty_buf,"%d",cart_items[cart_count-1].qty);
			cart_items[cart_count-1].count=1;
			sprintf(tx_buf,"#%s,%d,%d$",cart_items[cart_count-1].name,cart_items[cart_count-1].price,cart_items[cart_count-1].qty);
		}
		}
	}
	/* Splice the updated quantity into the in-memory copy of
	 * stock.csv, growing/shrinking as needed if the digit count changed. */
	p=strstr(file_buff,card);
	if(p==NULL)
	{
		printf("OOPs!Item not found\n\n");
		//exit(0);
		return;
	}
	p=p+9;
	if(strlen(qty_buf)==qty_len)
	{
		strncpy(p,qty_buf,strlen(qty_buf));
		//memmove(p,p+l,strlen(p+l)+1);
	}
	else
	{
		memmove(p,p+1,strlen(p+1)+1);
		posn=p-file_buff;
		file_buff=realloc(file_buff,(strlen(file_buff))+1);
		//strncpy(file_buff+posn,qty_buf,strlen(qty_buf));
		memcpy(file_buff+posn,qty_buf,strlen(qty_buf));
	}
	/* Write the updated stock data back to disk. */
	rewind(fp);
	fprintf(fp,"%s",file_buff);
	rewind(fp);
	/* Report the scanned item's details back to the microcontroller. */
	uart_send(tx_buf);
	fclose(fp);
	puts("\n------CART ITEMS------\n");
	for(int i=0;i<cart_count;i++)
	{
		printf("Item name:%s Price:%d Remaining QTY:%d\n",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
	}
	puts("\n");
	//printf("%s\n",file_buff);
}
/* Handles a customer re-scanning an item's card to remove one unit from
 * the cart. If the item's in-cart count reaches zero, it is removed
 * from cart_items entirely. Restores one unit back to stock.csv's
 * quantity field (in place) and notifies the microcontroller of either
 * the updated item state or that the item wasn't found in the cart. */
void delete_item(char *card)
{
	char *itemname=NULL,price_buf[12],qty_buf[12],tx_buf[100];
	int flag=0,qty_len;
	for(int i=0;i<cart_count;i++)
	{
		if(strcmp(cart_items[i].card,card)==0)
		{
			/* Found the item in the cart: give back one unit of stock
			 * and decrement its in-cart count. */
			--cart_items[i].count;
			qty_len=snprintf(NULL,0,"%d",cart_items[i].qty);
			++cart_items[i].qty;
			sprintf(qty_buf,"%d",cart_items[i].qty);
			sprintf(tx_buf,"#%s,%d,%d$",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
			if(cart_items[i].count==0)
			{
				/* No more units of this item left in the cart: remove
				 * its entry by shifting the later entries down. */
				memmove(&cart_items[i],&cart_items[i+1],(cart_count-i-1)*sizeof(struct cart));
				--cart_count;
			}
			flag=1;
			break;
		}
	}
	if(flag)
	{
		puts("\n------CART ITEMS------\n");
		for(int i=0;i<cart_count;i++)
		{
		printf("Item name:%s Price:%d Remaining QTY:%d\n",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
		}
		puts("\n");

		int posn;
		char *file_buff=NULL,*p=NULL;
		file_buff=openfile("stock.csv","r+",file_buff);
		p=strstr(file_buff,card);
		if(p==NULL)
		{
			printf("Item not found\n");
			//exit(0);
			return;
		}
		p=p+9;
		if(strlen(qty_buf)==qty_len)
		{
			strncpy(p,qty_buf,strlen(qty_buf));
			//memmove(p,p+l,strlen(p+l)+1);
		}
		else
		{
			/* Splice the (differently-sized) restored quantity into
			 * the in-memory copy of stock.csv. */
			posn=p-file_buff;
			file_buff=realloc(file_buff,(strlen(file_buff)+1)+1);
			p=file_buff+posn;
			memmove(p+1,p,strlen(p)+1);
			//file_buff=realloc(file_buff,(strlen(file_buff)+1)+1);
			//strncpy(file_buff+posn,qty_buf,strlen(qty_buf));
			memcpy(file_buff+posn,qty_buf,strlen(qty_buf));
		}
		fprintf(fp,"%s",file_buff);
		fclose(fp);
		//printf("%s\n",file_buff);	
		/* Notify the microcontroller of the item's updated state. */
		uart_send(tx_buf);
	}
	else
	{
		/* This card wasn't in the cart - tell the microcontroller so it
		 * can show an appropriate message. */
		printf("NO such item found\n");
		uart_send("#NO item$");
	}
}
/* Looks up a scanned bank/ATM card in bank.csv and reports back to the
 * microcontroller whether it was found ("#FOUND$") or not
 * ("#NOT FOUND$"). On success, caches the card's PIN/balance in
 * bank_details for the subsequent verify_pin() call. */
void verify_Bankcard(char *cardbuff)
{
	fp=fopen("bank.csv","r");
	char line[100];	
	int card_present=0;
	while(fgets(line,sizeof(line),fp))
	{
		if(strstr(line,cardbuff))
		{
			sscanf(line,"%[^,],%d,%d",bank_details.card,&bank_details.pin,&bank_details.bal);
			card_present=1;
			break;
		}
	}
	fclose(fp);
	if(card_present==1)
	{
		uart_send("#FOUND$");
		puts("CARD FOUND");
	}
	else 
	{
		uart_send("#NOT FOUND$");
		puts("NOT CARD FOUND");
	}
	//printf("card:%s pin:%d bal:%d\n",bank_details.card,bank_details.pin,bank_details.bal);
}
/* Verifies the entered PIN against the previously looked-up
 * bank_details (from verify_Bankcard) and, if correct and the balance
 * is sufficient, deducts `amt` from the balance. Rewrites bank.csv line
 * by line into temp.csv (updating only the matching card's row), then
 * atomically replaces bank.csv with temp.csv. Reports the outcome back
 * to the microcontroller as one of:
 *   "#SUCCESS$" - PIN correct and payment deducted
 *   "#LOWBAL$"  - PIN correct but insufficient balance
 *   "#WRONGPIN$" - PIN incorrect */
void verify_pin(int pin,int amt)
{
	char line[100];
	//static int cnt;
	fp=fopen("bank.csv","r");
	FILE *temp_fp=fopen("temp.csv","w");
	while(fgets(line,sizeof(line),fp))
	{
		if(strstr(line,bank_details.card))
		{
			if(pin==bank_details.pin)
			{
				if(bank_details.bal>=amt)
				{
					bank_details.bal-=amt;
					uart_send("#SUCCESS$");
					puts("PAYMENT SUCCESSFULL");
				}
				else {
					puts("INSUFFICIENT FUNDS IN ACCOUNT");
					uart_send("#LOWBAL$");
				}
			}
			else{
				uart_send("#WRONGPIN$");
				puts("WRONG PIN!!");
			}
			/* Write this card's row (with balance updated if the
			 * deduction succeeded) into the temp file. */
			fprintf(temp_fp,"%s,%d,%d\n",bank_details.card,bank_details.pin,bank_details.bal);
		}
		else
			/* Not the matching card: copy the row through unchanged. */
			fputs(line,temp_fp);
	}
	fclose(fp);
	fclose(temp_fp);
	/* Atomically replace bank.csv with the updated temp.csv. */
	remove("bank.csv");
	rename("temp.csv","bank.csv");
}
/* Finalises a transaction once the microcontroller reports the outcome:
 *   msg[0]=='S' -> Sale completed. Logs the amount + timestamp to
 *                  income.csv, adds it to the running `total`, and
 *                  clears the in-memory cart.
 *   msg[0]=='C' -> Purchase cancelled. Restores each cart item's
 *                  quantity back into stock.csv (by rewriting the file
 *                  line by line into temp.csv), then clears the cart. */
void transaction_result(char *msg)
{
	int flag;
	char line[100];
	FILE *t_fp=fopen("temp.csv","w");
	if(msg[0]=='S')
	{
		time_t t;
		struct tm *tm_info;
		time(&t);
		tm_info=localtime(&t);
		int amt=atoi(msg+1);
		total+=amt;
		char datetime[30];
		printf("Total amt:%s\n",msg+1);
		strftime(datetime,sizeof(datetime),"%d-%m-%y,%H:%M:%S",tm_info);
		fp=fopen("income.csv","a");
		fprintf(fp,"%s,%d\n",datetime,amt);
		fclose(fp);
		/* Sale complete: clear the cart for the next customer. */
		free(cart_items);
		cart_items=NULL;
		 cart_count=0;
	}
	else if(msg[0]=='C')
	{
		printf("In cancel\n");
		fp=fopen("stock.csv","r");
		 while(fgets(line,sizeof(line),fp))
		 {
			 flag=1;
			/* If this stock row matches a cart item, write back its
			 * quantity plus whatever was held in the cart (restoring
			 * the stock as if it was never taken). */
			for(int i=0;i<cart_count;i++)
			{
				if(strstr(line,cart_items[i].card))
				{
					fprintf(t_fp,"%s,%s,%d,%d\n",cart_items[i].name,cart_items[i].card,(cart_items[i].qty+cart_items[i].count),cart_items[i].price);
					flag=0;
					break;
				}
			}
			/* Otherwise, copy the row through unchanged. */
			if(flag)
				fputs(line,t_fp);
		 }
		 /* Cancellation complete: clear the cart. */
		 free(cart_items);
		 cart_items=NULL;
		 cart_count=0;
		 fclose(fp);
		 fclose(t_fp);
		 /* Atomically replace stock.csv with the restored temp.csv. */
		 remove("stock.csv");
		 rename("temp.csv","stock.csv");
		 puts("PURCHASE CANCELED");
	}
}
/* Opens the serial device and blocks reading bytes into `buff` until a
 * '\0' terminator byte is received (matching how U0_send() on the
 * microcontroller side always appends a trailing '\0'). Returns 1 if
 * the serial device could not be opened (note: no explicit return value
 * is given on the success path - kept as-is, not modified). */
int uart_receive(char *buff)
{
	char rx;
	int i=0;
	if((fd = serialOpen ("/dev/ttyUSB0",9600)) < 0)
	{
		fprintf (stderr, "Unable to open serial device: %s\n", strerror (errno)) ;
                return 1 ;
	}
	//puts("serial port is opened\n");
	while(1)
	{
		rx=serialGetchar(fd);
		if(rx=='\0')
		{
			buff[i]='\0';
			break;
		}
		else
		{
			buff[i++]=rx;
		}
	}
}
/* Fetches the next incoming message into `buff`, either from the real
 * serial link, or (if TEST_MODE is defined at compile time) by manually
 * typing a card number at the console for easier testing without the
 * actual hardware attached. */
void get_data(char *buff)
{
#ifdef TEST_MODE
	printf("Test Mode\nEnter Card Number Manually:");
	__fpurge(stdin);
	fgets(buff,30,stdin);
	buff[strlen(buff)-1]='\0';
#else
	uart_receive(buff);
#endif
}
int main()
{
	char buff[30],temp[12];
	int i;
	//uart_receive(buff);
	/* Main receive loop: waits for each message from the microcontroller
	 * and dispatches it based on its leading character / content
	 * (see the message protocol summary in the file header comment). */
	while(1)
	{
		puts("Ready To Recieve...");
		get_data(buff);
		//printf("%s\n",buff);
		if(strcmp(buff,manager_card)==0)
		{
			/* Manager card scanned: open the stock-management menu. */
			//printf("manager card\n");
			manager_fun();
		}
		else if(buff[0]=='D'&&buff[9]=='$')
		{
			/* "D<card>$" - delete this item from the cart. */
			//printf("Delete item\n");
			buff[strlen(buff)-1]='\0';
			//printf("%s\n",buff+1);
			delete_item(buff+1);
		}
		else if(buff[0]=='B')
		{
			/* "B<card>A<amt>$" - verify a bank card for checkout. Only
			 * the card portion (up to the 'A') is needed here; the
			 * amount is used later in verify_pin(). */
			char *p=buff;
			//printf("Verify card\n");
			buff[strlen(buff)-1]='\0';
			while(*p!='A')
			{
				++p;
			}
			*p='\0';
			//printf("%s\n%s\n",buff+1,p+1);
			verify_Bankcard(buff+1);
		}
		else if(buff[0]=='P')
		{
			/* "P<pin>A<amt>$" - verify PIN and attempt to deduct the
			 * amount from the previously-verified bank card. */
			char *p=buff;
			buff[strlen(buff)-1]='\0';
			while(*p!='A')
			{
				++p;
			}
			*p='\0';
			verify_pin(atoi(buff+1),atoi(p+1));
			//printf("%s\n",buff+1);
		}
		else if(buff[0]=='S'||buff[0]=='C')
		{	
			/* "S<amt>$" or "C$" - transaction finished (sale or cancel). */
			buff[strlen(buff)-1]='\0';
			transaction_result(buff);
		}
		else if(strcmp(buff,"exit")==0)
		{
			/* Shut down: log the day's running total and exit. */
			fp=fopen("income.csv","a");
			fprintf(fp,"%s,,%d\n","TOTAL",total);
			fclose(fp);
			exit(0);
		}
		else 
		{	
			/* Anything else is treated as a plain item card scan. */
			printf("item card\n");
			item_card(buff);
		}
	}
}
