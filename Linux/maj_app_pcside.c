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
 *   "R<card ID>"             - treated as an item card scan (add to cart)
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
#include<signal.h>

/* When TEST_MODE is defined, card data is entered manually from the
   keyboard instead of being read from the UART/serial hardware. */

// Uncomment to enable testing without the LPC2148 controller.
//#define TEST_MODE 

/* Card number that unlocks the manager menu (stock entry/update). */
// Manager RFID Card ID
// Replace "12608170" with your manager's RFID card ID before running the project.
char manager_card[10] = "12608170";

/* Represents one item currently in a customer's shopping cart. */
struct cart
{
	char name[50];   // item name
	char card[10];   // item's RFID/card id (used as its "SKU")
	int qty;         // remaining stock quantity (from stock.csv) after last scan
	int price;       // item price
	int count;       // how many units of this item are currently in the cart
};

/* Dynamic array of cart items, grown with realloc as items are scanned. */
struct cart *cart_items=NULL;

/* Holds the bank record (card, pin, balance) currently loaded for a payment. */
struct bank
{
	char card[10];
	int pin;
	int bal;
}bank_details;

int cart_count=0;   // number of items currently in cart_items[]
int total,income;   // total = running cost of current cart, income = accumulated income across sales
int fd;             // serial file descriptor used for UART communication
FILE *fp;            // shared FILE* used across functions for stock/bank/income files

/* Sends a null-terminated string out over the serial/UART connection,
   one character at a time. */
void uart_send(char *p)
{
	printf("send\n");
	while(*p)
	{
		serialPutchar (fd,*p);
		++p;
	}
}

/* Returns 0 if the given string contains only digit characters,
   otherwise returns 1 (i.e. "is this NOT a valid integer string"). */
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

/* Opens "filename" with the given mode, reads its entire contents into
   a newly-allocated, null-terminated buffer, and returns that buffer.
   Also leaves the global fp pointing at the (rewound) open file so the
   caller can later fprintf() back into it.
   NOTE: file_buff parameter is unused for output (it's passed by value),
   the allocated buffer is only returned via the return value. */
void *openfile(char *filename,char *mode,char *file_buff)
{
	fp=fopen(filename,mode);
	if(fp==NULL)
	{
		perror("fopen");
		//return 0;
		exit(0);
	}
	fseek(fp,0,2);              // seek to end of file
	int size=ftell(fp)+1;       // file size (+1 for null terminator)
	rewind(fp);                 // back to start
	file_buff=calloc(size,1);   // allocate zero-initialized buffer
	fread(file_buff,size-1,1,fp);
	rewind(fp);                 // leave file position back at start for caller
	file_buff[size-1]='\0';
	return file_buff;
}

/* Manager-only menu: lets an operator add a new stock item, or update
   the price/quantity of an existing item in stock.csv. Runs in a loop
   until the operator chooses "Back". */
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
		__fpurge(stdin);   // clear any leftover input in stdin buffer
		puts("+=+=+=+=+=+=+=+=+=+=+=+=+=+=+MENU+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+");
		printf("1.Entry\n2.Update price\n3.Update Quantity\n4.Back\n");
		printf("Select choice:");
		scanf("%c",&c);
		switch(c)
		{
			/* ---- Option 1: append a brand-new item to stock.csv ---- */
			case '1':fp=fopen("stock.csv","a");
				if(fp==NULL)
				{
					perror("fopen\n");
					return ;
					//exit(0);
				}
				__fpurge(stdin);
				printf("enter item name:");
				fgets(item_name,50,stdin);
				item_name[strlen(item_name)-1]='\0';   // strip trailing newline from fgets
				card1:printf("Enter the Card Number:");
				__fpurge(stdin);
				fgets(item_card,10,stdin);
				item_card[strlen(item_card)-1]='\0';
				// Card number must be exactly 8 numeric digits; otherwise retry
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
				// Write new "name,card,qty,price" record as a new line in stock.csv
				fprintf(fp,"%s,%s,%d,%d\n",item_name,item_card,qty,price);
				fclose(fp);
				break;

			/* ---- Option 2: update the price of an existing item ---- */
			case '2':
				// Load the whole stock.csv file into memory so we can edit it in-place
				file_buff=openfile("stock.csv","r+",file_buff);
				__fpurge(stdin);
				card2:printf("Enter card Number to Update price\n");
				fgets(item_card,10,stdin);
				item_card[strlen(item_card)-1]='\0';
				if((strlen(item_card)!=8)||checkint(item_card))
				{
					printf("Enter 8 digits Card number\n");
					goto card2;
				}
				// Locate the line containing this card number in the buffer
				p=strstr(file_buff,item_card);
				if(p==NULL)
				{
					printf("Item not found\n");
					//exit(0);
					return;
				}
				p=p+9;   // skip past "cardnumber," (8 digits + comma) to reach the qty field
				q=p;
				// skip over the quantity field to find the start of the price field
				while(*q!=',')
				{
					++q;
				}
				++q;   // q now points at start of price field
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
				total_len=0;l1=0;l2=0;
				k=q;
				// l1 = length of the OLD price text (up to end of line)
				while(*k!='\n')
				{
					++l1;
					++k;
				}
				l2=strlen(price);       // length of the NEW price text
				total_len=strlen(file_buff);
				posn=q-file_buff;       // offset of the price field within the buffer
				// Grow or shrink the buffer depending on whether the new price
				// string is longer or shorter than the old one, shifting the
				// remaining file contents accordingly (in-memory splice).
				if(l2>l1)
				{
					file_buff=realloc(file_buff,total_len+(l2-l1)+1);
					k=file_buff+posn;
					memmove(k+(l2-l1),k,strlen(k)+1);
				}
				else if(l2<l1)
				{
					k=file_buff+posn;
					memmove(k,k+(l1-l2),strlen(k+(l1-l2))+1);
					file_buff=realloc(file_buff,total_len-(l1-l2)+1);
				}
				q=file_buff+posn;
				i=0;
				// Copy the new price digits into the (now correctly-sized) gap
				while(price[i]!='\0')
				{
					*q=price[i++];
					++q;
				}
				// Write the modified buffer back out to stock.csv
				fprintf(fp,"%s",file_buff);
				fclose(fp);
				break;

			/* ---- Option 3: update the quantity of an existing item ---- */
			case '3':
				file_buff=openfile("stock.csv","r+",file_buff);
                                __fpurge(stdin);
				card3:printf("Enter card Number to Update Quantity\n");
                                fgets(item_card,10,stdin);
                                item_card[strlen(item_card)-1]='\0';
				if((strlen(item_card)!=8)||checkint(item_card))
				{
					printf("Enter 8 digits Card number\n");
					goto card3;
				}
                                p=strstr(file_buff,item_card);
                                if(p==NULL)
                                {
                                        printf("Item not found\n");
                                        //exit(0);
					return;
                                }
                                p=p+9;   // skip "cardnumber," to reach start of qty field
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
                                // l1 = length of the OLD quantity text
                                while(*q!=',')
                                {
                                        ++l1;
                                        ++q;
                                }
                                l2=strlen(qty);       // length of the NEW quantity text
                                total_len=strlen(file_buff);
                                posn=p-file_buff;      // offset of the qty field within the buffer
                                // Same in-memory splice technique as the price-update case above
                                if(l2>l1)
                                {
                                        file_buff=realloc(file_buff,total_len+(l2-l1)+1);
                                        q=file_buff+posn;
                                        memmove(q+(l2-l1),q,strlen(q)+1);
                                }
                                else if(l2<l1)
                                {
                                        q=file_buff+posn;
                                        memmove(q,q+(l1-l2),strlen(q+(l1-l2))+1);
                                        file_buff=realloc(file_buff,total_len-(l1-l2)+1);
                                }
                                q=file_buff+posn;
                                i=0;
                                // Copy the new quantity digits into the gap
                                while(qty[i]!='\0')
                                {
                                        *q=qty[i++];
                                        ++q;
                                }
                                fprintf(fp,"%s",file_buff);
                                fclose(fp);
                                break;

			/* ---- Option 4: leave the manager menu ---- */
			case '4':return;
		}
	}
}

/* Called when a customer scans an item's card. Looks the item up in
   stock.csv, adds/updates it in the in-memory cart (cart_items[]),
   decrements available stock by one, writes the updated stock back to
   stock.csv, and sends a transaction message back over UART. */
void item_card(char *card)
{
	char line[100],qty_buf[12],tx_buf[80];
	char *p=NULL;
	int flag=0,qty_len=0,posn;
	char *file_buff=NULL;
	// Load stock.csv into memory (also opens fp for later re-writing)
	file_buff=openfile("stock.csv","r+",file_buff);
	rewind(fp);
	while(fgets(line,sizeof(line),fp))
	{
		if(strstr(line,card))
		{
			// Check whether this item is already present in the cart
			for(int i=0;i<cart_count;i++)
			{
			//	printf("card=%s line=%s\n",card,line);
				if(strcmp(cart_items[i].card,card)==0)
				{
					// Item already in cart: refresh its data from stock.csv,
					// then decrement remaining stock and increment cart count
					sscanf(line,"%[^,],%[^,],%d,%d",cart_items[i].name,cart_items[i].card,&cart_items[i].qty,&cart_items[i].price);
					if(cart_items[i].qty==0)
					{
						if(cart_items[i].count==0)
						{
							// Nothing of this item in cart and none in stock: remove it
							--cart_count;
							return;
						}
						printf("\n*****%s is out of stock*****\n\n",cart_items[i].name);
						goto cart;
						return;
					}
					qty_len=snprintf(NULL,0,"%d",cart_items[i].qty);   // digit-count of qty before decrement
					--cart_items[i].qty;
					sprintf(qty_buf,"%d",cart_items[i].qty);
					++cart_items[i].count;
					sprintf(tx_buf,"#%s,%d,%d$",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
					total+=cart_items[i].price;
					flag=1;
					break;
				}
			}
		if(flag==0)
		{
			// Item not yet in cart: add a brand-new cart_items[] entry for it
			puts(line);
			++cart_count;
			cart_items=realloc(cart_items,cart_count*sizeof(struct cart));
			memset(&cart_items[cart_count-1], 0, sizeof(struct cart));
			sscanf(line,"%[^,],%[^,],%d,%d",cart_items[cart_count-1].name,cart_items[cart_count-1].card,&cart_items[cart_count-1].qty,&cart_items[cart_count-1].price);
				if(cart_items[cart_count-1].qty==0)
				{
					//--cart_count;
					printf("\n*****%s is out of stock*****\n\n",cart_items[cart_count-1].name);
					if(cart_items[cart_count-1].count==0)
					{
						--cart_count;
						return;
					}
					goto cart;
					return;
				}
			//else ++cart_count; 
			qty_len=snprintf(NULL,0,"%d",cart_items[cart_count-1].qty);
			--cart_items[cart_count-1].qty;
			sprintf(qty_buf,"%d",cart_items[cart_count-1].qty);
			cart_items[cart_count-1].count=1;
			sprintf(tx_buf,"#%s,%d,%d$",cart_items[cart_count-1].name,cart_items[cart_count-1].price,cart_items[cart_count-1].qty);
			total+=cart_items[cart_count-1].price;
		}
		}
	}
	// Now patch the new quantity value back into the in-memory file_buff
	// at the matching card's record, and rewrite stock.csv from it.
	p=strstr(file_buff,card);
	if(p==NULL)
	{
		printf("OOPs!Item not found\n\n");
		return;
	}
	p=p+9;   // skip past "cardnumber," to the qty field
	if(strlen(qty_buf)==qty_len)
	{
		// New quantity has the same digit-width as before: overwrite in place
		strncpy(p,qty_buf,strlen(qty_buf));
	}
	else
	{
		// New quantity is a different width (e.g. "10" -> "9"): shrink the
		// buffer by one byte, shifting the remainder left, then copy in
		memmove(p,p+1,strlen(p+1)+1);
		posn=p-file_buff;
		file_buff=realloc(file_buff,(strlen(file_buff))+1);
		memcpy(file_buff+posn,qty_buf,strlen(qty_buf));
	}
	rewind(fp);
	fprintf(fp,"%s",file_buff);
	rewind(fp);
	uart_send(tx_buf);
	fclose(fp);
	// Print the current state of the cart to the console
	cart:puts("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=CART ITEMS=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
	puts("ITEM\t\t\tPRICE\t\tItems in CART\t\tREMAINING STOCK\n");
	for(int i=0;i<cart_count;i++)
	{
		printf("%-25s%-20d%-20d%d\n\n",cart_items[i].name,cart_items[i].price,cart_items[i].count,cart_items[i].qty);
	}
	printf("-------------------------------------Total Amt:%d------------------------------------\n",total);
	//printf("%s\n",file_buff);
}

/* Called when a customer scans a "delete/remove from cart" action for a
   given card. Decrements that item's cart count, puts one unit back
   into available stock, updates stock.csv, and prints the cart. */
void delete_item(char *card)
{
	if(cart_count==0)
	{
		puts("\n********CART is EMPTY********\n\n");
		return;
	}
	char *itemname=NULL,price_buf[12],qty_buf[12],tx_buf[100];
	int flag=0,qty_len;
	for(int i=0;i<cart_count;i++)
	{
		if(strcmp(cart_items[i].card,card)==0)
		{
			--cart_items[i].count;                              // one fewer of this item in the cart
			qty_len=snprintf(NULL,0,"%d",cart_items[i].qty);     // digit-count of qty before increment
			++cart_items[i].qty;                                // one more unit back in stock
			sprintf(qty_buf,"%d",cart_items[i].qty);
			sprintf(tx_buf,"#%s,%d,%d$",cart_items[i].name,cart_items[i].price,cart_items[i].qty);
			if(cart_items[i].count==0)
			{
				// No more of this item in the cart: remove its entry entirely
				memmove(&cart_items[i],&cart_items[i+1],(cart_count-i-1)*sizeof(struct cart));
				--cart_count;
			}
			flag=1;
			total-=cart_items[i].price;
			break;
		}
	}
	if(flag)
	{
		// Item was found in cart: print updated cart contents
		puts("\n=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=CART ITEMS=-=-=-=-=-=-=-=-=-=--=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=\n");
		puts("ITEM\t\t\tPRICE\t\tItems in CART\t\tREMAINING STOCK\n");
		if(cart_count==0)
			puts("\n********CART is EMPTY********\n\n");
		else{
		for(int i=0;i<cart_count;i++)
		{
			printf("%-25s%-20d%-20d%d\n",cart_items[i].name,cart_items[i].price,cart_items[i].count,cart_items[i].qty);
		}
		printf("-------------------------------------Total Amt:%d------------------------------------\n",total);
		}
			//if(cart_count==0)
			//	puts("\n********CART is EMPTY********\n\n");

		int posn;
		char *file_buff=NULL,*p=NULL;
		// Load stock.csv, find the item's record, and patch the quantity
		// field back in (same shrink/grow logic as in item_card()).
		file_buff=openfile("stock.csv","r+",file_buff);
		p=strstr(file_buff,card);
		if(p==NULL)
		{
			printf("Item not found\n");
			return;
		}
		p=p+9;
		if(strlen(qty_buf)==qty_len)
		{
			strncpy(p,qty_buf,strlen(qty_buf));
		}
		else
		{
			posn=p-file_buff;
			file_buff=realloc(file_buff,(strlen(file_buff)+1)+1);
			p=file_buff+posn;
			memmove(p+1,p,strlen(p)+1);
			memcpy(file_buff+posn,qty_buf,strlen(qty_buf));
		}
		fprintf(fp,"%s",file_buff);
		fclose(fp);
		uart_send(tx_buf);
	}
	else
	{
		// Card scanned for deletion doesn't match anything in the cart
		printf("\n<-<-<-<-<-<-NO such item FOUND in CART->->->->->->\n\n");
		uart_send("#NO item$");
	}
}

/* Looks up a bank card number in bank.csv and reports (via UART and
   console) whether it was found, storing the matched record in the
   global bank_details struct for later use by verify_pin(). */
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
		puts("$$$$$$CARD FOUND$$$$$$");
	}
	else 
	{
		uart_send("#NOT FOUND$");
		puts("<-<-<-<-<-<-NOT CARD FOUND->->->->->->");
	}
}

/* Verifies the PIN entered for the currently-loaded bank_details card
   and, if correct and funds are sufficient, deducts "amt" from the
   balance. Rewrites bank.csv with the updated balance regardless
   (via a temp file + rename), since the whole file must be rewritten
   to update one line in a plain CSV. */
void verify_pin(int pin,int amt)
{
	char line[100];
	fp=fopen("bank.csv","r");
	FILE *temp_fp=fopen("temp.csv","w");
	while(fgets(line,sizeof(line),fp))
	{
		if(strstr(line,bank_details.card))
		{
			// This is the matching customer's record
			if(pin==bank_details.pin)
			{
				if(bank_details.bal>=amt)
				{
					bank_details.bal-=amt;
					uart_send("#SUCCESS$");
					puts("$$$$$$PAYMENT SUCCESSFULL$$$$$$");
				}
				else {
					puts("@@@@@INSUFFICIENT FUNDS IN ACCOUNT@@@@@");
					uart_send("#LOWBAL$");
				}
			}
			else{
				uart_send("#WRONGPIN$");
				puts("!!!!!xxxWRONG PINxxx!!!!!");
			}
			// Write back this record (balance only actually changes on success)
			fprintf(temp_fp,"%s,%d,%d\n",bank_details.card,bank_details.pin,bank_details.bal);
		}
		else
			// Not the matching record: copy the line through unchanged
			fputs(line,temp_fp);
	}
	fclose(fp);
	fclose(temp_fp);
	// Replace bank.csv with the newly-written temp.csv
	remove("bank.csv");
	rename("temp.csv","bank.csv");
}

/* Finalizes a transaction. msg[0]=='S' means a successful sale (payment
   completed): log the income and clear the cart. msg[0]=='C' means a
   cancel/shutdown: write back any items still "checked out" in the
   cart to stock.csv (restoring their quantities) and log any pending
   income before shutting down. */
void transaction_result(char *msg)
{
	int flag;
	char line[100];
	FILE *t_fp=fopen("temp.csv","w");
	if(msg[0]=='S')
	{
		// ---- Successful sale: record income with a timestamp ----
		time_t t;
		struct tm *tm_info;
		time(&t);
		tm_info=localtime(&t);
		int amt=atoi(msg+1);
		income+=total;
		total=0;
		char datetime[30];
		printf("\n$$$TOTAL INCOME$$$:%s\n\n",msg+1);
		strftime(datetime,sizeof(datetime),"%d-%m-%y,%H:%M:%S",tm_info);
		fp=fopen("income.csv","a");
		fprintf(fp,"%s,%d\n",datetime,amt);
		fclose(fp);
		free(cart_items);
		cart_items=NULL;
		 cart_count=0;
	}
	else if(msg[0]=='C')
	{
		// ---- Cancel/shutdown: rebuild stock.csv, restoring quantities
		//      of anything still held in the cart (qty+count), then
		//      log total accumulated income and shut down. ----
		fp=fopen("stock.csv","r");
		 while(fgets(line,sizeof(line),fp))
		 {
			 flag=1;
			for(int i=0;i<cart_count;i++)
			{
				if(strstr(line,cart_items[i].card))
				{
					// Restore this item's original quantity (qty currently in
					// stock plus however many are held in the cart)
					fprintf(t_fp,"%s,%s,%d,%d\n",cart_items[i].name,cart_items[i].card,(cart_items[i].qty+cart_items[i].count),cart_items[i].price);
					flag=0;
					break;
				}
			}
			if(flag)
				fputs(line,t_fp);
		 }
		 free(cart_items);
		 cart_items=NULL;
		 cart_count=0;
		 fclose(fp);
		 fclose(t_fp);
		 remove("stock.csv");
		 rename("temp.csv","stock.csv");
		 if(income>0)
		 {
			fp=fopen("income.csv","a");
                        fprintf(fp,"%s,,%d\n","TOTAL",income);
                        fclose(fp);
		 }
		 //puts("\nxxxxxxPURCHASE CANCELEDxxxxxx\n");
		 puts("\nxxxxxx!!!SHUTTING DOWN!!!xxxxxx\n");
	}
}

/* Reads a null-terminated message from the UART/serial device into
   buff, one character at a time, stopping at a '\0' byte. */
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

/* Fetches the next incoming "card scan" message into buff. In
   TEST_MODE, the operator types it in manually from the keyboard;
   otherwise it's read from the real UART hardware. */
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

/* Reads stock.csv and prints a formatted table of all available items,
   splitting each comma-separated line into aligned columns. */
void display_stock(void)
{
	int i;
	char line[100],*p=NULL;
	fp=fopen("stock.csv","r");
	if(fp==NULL)
	{
		puts("stock.csv not found\n");
		return;
	}
	printf("\n===================================================================================\n");
	printf("                             AVALIABLE STOCK ITEMS                      \n\n\n");
	printf("ITEM\t\t\tID\t\t\tStock\t\t\tPrice\n");
	printf("====================================================================================\n");
	while(fgets(line,100,fp))
	{
		p=line;
		char *q=p;
		// Replace each ',' with '\0' in place so each field can be
		// printed individually as a column, then advance to the next field
		while(*p)
		{
			if(*p==',')
			{
				*p='\0';
				printf("%-20s\t",q);
				q=p+1;
			}
			++p;
		}
		printf("%s\n",q);   // print the last field (price)
	}
	fclose(fp);
}

/* Signal handler for SIGINT/SIGQUIT: treat an interrupt as a cancel,
   restoring stock and shutting down cleanly instead of just dying. */
void sig_handler()
{
	 transaction_result("C");
	 exit(0);
}

/* Registers sig_handler() for Ctrl+C (SIGINT) and Ctrl+\ (SIGQUIT) so
   the program can shut down gracefully on those signals. */
void init_signal()
{
	signal(SIGINT,sig_handler);
	//signal(SIGSTOP,sig_handler);
	signal(SIGQUIT,sig_handler);
}

/* Main event loop: shows the stock table once at startup, then
   repeatedly waits for an incoming "card scan" message and dispatches
   it based on its first character(s):
     manager_card -> open manager menu
     "D<card>$"   -> delete_item (remove one unit from cart)
     "B<card>A"   -> verify_Bankcard (bank card lookup)
     "P<pin>A<amt>" -> verify_pin (PIN + payment amount)
     "S..." / "C..." -> transaction_result (sale / cancel)
     "exit"       -> log income and quit
     "R<card>"    -> item_card (add/scan item into cart) */
int main()
{
	char buff[30],temp[12];
	int i,display=0;
	init_signal();
	while(1)
	{
		if(display==0)
		{
			display_stock();
			display=1;
		}
		puts("Ready To Recieve...");
		get_data(buff);
		if(strcmp(buff,manager_card)==0)
		{
			manager_fun();
		}
		else if(buff[0]=='D'&&buff[9]=='$')
		{
			buff[strlen(buff)-1]='\0';   // strip trailing '$'
			delete_item(buff+1);         // pass card number (skip leading 'D')
		}
		else if(buff[0]=='B')
		{
			// Format: "B<card>A..." - extract the card number between 'B' and 'A'
			char *p=buff;
			buff[strlen(buff)-1]='\0';
			while(*p!='A')
			{
				++p;
			}
			*p='\0';
			verify_Bankcard(buff+1);
		}
		else if(buff[0]=='P')
		{
			// Format: "P<pin>A<amount>" - split on 'A' into pin and amount
			char *p=buff;
			buff[strlen(buff)-1]='\0';
			while(*p!='A')
			{
				++p;
			}
			*p='\0';
			verify_pin(atoi(buff+1),atoi(p+1));
		}
		else if(buff[0]=='S'||buff[0]=='C')
		{	
			buff[strlen(buff)-1]='\0';
			transaction_result(buff);
		}
		else if(strcmp(buff,"exit")==0)
		{
			// Log any pending income total before quitting
			if(income>0)
			{
			fp=fopen("income.csv","a");
			fprintf(fp,"%s,,%d\n","TOTAL",income);
			fclose(fp);
			}
			exit(0);
		}
		else if(buff[0]=='R')
		{	
			item_card(buff+1);   // scan/add item to cart (skip leading 'R')
		}
	}
}
