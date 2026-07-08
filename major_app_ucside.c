/*
 * major_app_ucside.c
 * -----------------------------------------------------------------------
 * Main application for the microcontroller side of a "Smart Cart" RFID
 * shopping-cart system.
 *
 * Flow:
 *   1. User presses the Entry button (eint0) to start a session.
 *   2. Each item's RFID card is scanned (received via UART1 -> `buff`)
 *      and forwarded to the PC side over UART0; the PC responds with
 *      the item's name/price/qty (received via UART0 -> `rx_buff`),
 *      which updates the running total and LCD display.
 *   3. The Delete button (eint1) lets the user remove the last scanned
 *      item by re-scanning its card.
 *   4. The Exit/Checkout button (eint2) starts the payment flow, which
 *      offers Cash or Card payment, talking to the PC side (which in
 *      turn handles the "bank"/stock CSV files) to validate PIN/
 *      balance and complete the transaction.
 *
 * Communication framing:
 *   - UART1 (RFID/keypad module) frames data between STX(0x02)/ETX(0x03).
 *   - UART0 (PC side) frames data between '#' and '$'.
 *   (See interrupt_isr.c for how buff/rx_buff get filled.)
 * -----------------------------------------------------------------------
 */
#include<lpc21xx.h>
#include<string.h>
#include<stdio.h>
#include"types.h"
#include"delay.h"
#include"lcd.h"
#include"lcd_defines.h"
#include"kpm.h"
#include"uart0_uart1.h"
#include"major_proj.h"
/* buff: raw RFID card ID received from UART1.
 * rfid: formatted outgoing message built around the card ID.
 * rx_buff: raw response received from the PC side over UART0.
 * msg: formatted outgoing message sent to the PC side.
 * item_name: name of the most recently looked-up item (parsed from rx_buff). */
s8 buff[30],rfid[30],rx_buff[50],msg[30],item_name[30];
s8 op;   /* currently selected payment-menu option ('1'=cash,'2'=card,'3'=cancel) */
/* Event flags, set by the ISRs (interrupt_isr.c) and cleared by the main
 * loop once handled:
 *   entry_flag  - Entry button pressed
 *   delete_flag - Delete button pressed
 *   exit_flag   - Exit/Checkout button pressed
 *   scan_flag   - a full RFID card ID has been received in `buff`
 *   recv_flag   - a full response has been received in `rx_buff` */
s32 entry_flag,delete_flag,exit_flag,scan_flag,recv_flag,flag,i,retry,done;
s32 cost,qty,totalamt,cash,PIN,attempt_cnt;

/* Sends a null-terminated string to the PC side over UART0, followed by
 * an explicit '\0' byte (used by the PC side's framing), and clears
 * scan_flag since we are about to wait for a fresh response. */
void U0_send(char *buff)
{
	scan_flag=0;
	U0_Txstring(buff);
	U0_TxByte('\0');	
}
/* Parses the most recent PC-side response (rx_buff) of the form
 * "name,cost,qty" into item_name/cost/qty, and clears recv_flag. */
void U0_recv()
{
	recv_flag=0;
	sscanf(rx_buff,"%[^,],%d,%d",item_name,&cost,&qty);
}
/* Redraws the LCD with the current item's name/remaining quantity on
 * line 1 and the running total amount on line 2. */
void item_details()
{
	cmdLCD(CLEAR_LCD);
	strLCD(item_name);
	charLCD(' ');
	strLCD("RQTY:");
	U32LCD(qty);
	cmdLCD(GOTO_LINE2_POS0);
	strLCD("TAmt:");
	s32LCD(totalamt);	
}
/* Cash payment flow: prompts the user to key in the amount tendered via
 * the keypad and compares it to the total due. On an exact match,
 * notifies the PC side of the successful sale ("S<amt>$"), shows a
 * success message, and resets the total. Otherwise sets retry=1 so the
 * caller re-shows the payment menu. */
void cash_payment()
{
	cmdLCD(CLEAR_LCD);
	strLCD("Enter amount:");
	cash=Readnum();
	if(cash==totalamt)
	{
		retry=0;
		done=1;
		sprintf(msg,"S%d$",totalamt);
		U0_send(msg);
		cmdLCD(CLEAR_LCD);
		strLCD("Payment Success");
		cmdLCD(GOTO_LINE2_POS0);
		strLCD("Visit Again");
		delay_s(2);
		/*cmdLCD(CLEAR_LCD);
		strLCD("PRESS Entry Key");
		cmdLCD(GOTO_LINE2_POS0);
		strLCD("To Begin Shopping");*/
		totalamt=0;
		delay_s(2);
	}
	else retry=1;
}
/* Card (bank) payment flow:
 *   1. Prompts the user to place their ATM card and waits for a scan.
 *   2. Sends "B<card>A<amt>$" to the PC side to verify the card exists;
 *      if not found, sets retry=1 and shows an error.
 *   3. Otherwise, gives up to 3 attempts to enter the correct PIN,
 *      sending "P<pin>A<amt>$" each time and handling the PC side's
 *      SUCCESS / WRONGPIN / LOWBAL responses accordingly. On success,
 *      notifies the PC side of the sale ("S<amt>$") and resets the total.
 *      On repeated wrong PIN or low balance, sets retry=1. */
void card_payment()
{
	cmdLCD(CLEAR_LCD);
	strLCD("place ATM card");
	scan_flag=0;
	while(scan_flag==0);
	snprintf(rfid,sizeof(rfid),"B%sA%d$",buff,totalamt);
	U0_send(rfid);
	while(recv_flag==0); 
	recv_flag=0;
	if(strcmp(rx_buff,"NOT FOUND")==0)
	{
		retry=1;
		cmdLCD(CLEAR_LCD);
		strLCD("CARD Not Found");
		delay_s(2);
	}
	else
	{
		attempt_cnt=3;
		while(1)
		{
			cmdLCD(CLEAR_LCD);
			strLCD("Enter ATM PIN");
			cmdLCD(GOTO_LINE2_POS0);
			PIN=ReadPIN();
			snprintf(msg,sizeof(msg),"P%dA%d$",PIN,totalamt);
			U0_send(msg);
			while(recv_flag==0); 
			recv_flag=0;
			U0_recv();
								 
			if(strcmp(rx_buff,"SUCCESS")==0)	
			{
				retry=0;
				done=1;
				delay_ms(500);
				sprintf(msg,"S%d$",totalamt);
				U0_send(msg);
				cmdLCD(CLEAR_LCD);
				strLCD("payment success");
				cmdLCD(GOTO_LINE2_POS0);	
				strLCD("Visit Again");
				delay_s(2);
				/*cmdLCD(CLEAR_LCD);
				strLCD("PRESS Entry Key");
				cmdLCD(GOTO_LINE2_POS0);
				strLCD("To Begin Shopping");*/
				totalamt=0;
				break;
			}
	 		else if(strcmp(rx_buff,"WRONGPIN")==0)
			{	
				/* Wrong PIN: decrement remaining attempts and show how
				 * many chances are left before looping to retry. */
				--attempt_cnt;
				cmdLCD(CLEAR_LCD);
				strLCD("wrong PIN");
				cmdLCD(GOTO_LINE2_POS0);
				strLCD("YOu have");
				U32LCD(attempt_cnt);
				strLCD("chances");
				delay_s(2);
			}	
			else if(strcmp(rx_buff,"LOWBAL")==0)
			{
				/* Insufficient funds on the card: abandon card payment
				 * and let the caller offer another payment method. */
				retry=1;
				cmdLCD(CLEAR_LCD);
				strLCD("Low Balance");
				cmdLCD(GOTO_LINE2_POS0);
				strLCD("payment failed");
				delay_s(2);
				cmdLCD(CLEAR_LCD);
				strLCD("Try Another Way");
				delay_s(2);
				break;
		}	 
			if(attempt_cnt==0)
			{
				/* All PIN attempts used up: abandon card payment. */
				retry=1;
				cmdLCD(CLEAR_LCD);
				strLCD("Lost chances");
				cmdLCD(GOTO_LINE2_POS0);
				strLCD("payment failed");
				delay_s(2);
				break;
			}
		}		
	}					
}
int main()
{
	/* Bring up all peripherals (LCD, keypad, UARTs, external interrupts). */
	init();
	strLCD("smart cart RFID");
	delay_s(3);
	cmdLCD(CLEAR_LCD);
	strLCD("PRESS Entry Key");
	cmdLCD(GOTO_LINE2_POS0);
	strLCD("& Begin Shopping");
	while(1)
	{
		/*cmdLCD(CLEAR_LCD);
		strLCD("PRESS Entry Key");
		cmdLCD(GOTO_LINE2_POS0);
		strLCD("To Begin Shopping");*/
		if(entry_flag)
		{
			/* Start of a new shopping session: reset all session state
			 * and event flags, then show the "place card" prompt. */
			entry_flag=0;
			//buff={0};
			memset(buff,0,sizeof(buff));
			scan_flag=0;
			recv_flag=0;
			delete_flag=0;
			exit_flag=0;
			cmdLCD(CLEAR_LCD);
			strLCD("Hello,Place card");
			cmdLCD(GOTO_LINE2_POS0);
			strLCD("to Add Items");
			//strLCD("waiting...");
			while(1)
			{				
				if(scan_flag)
				{
					/* A new item's RFID card was scanned: forward the
					 * card ID to the PC side for lookup, briefly pulse
					 * P0.2 (buzzer/indicator) to acknowledge the scan,
					 * then show the scanned card ID on the LCD. */
					U0_send(buff);
					IOSET0=1<<2;
					delay_ms(1000);
					IOCLR0=1<<2;
					cmdLCD(CLEAR_LCD);
					strLCD("card scanned");
					cmdLCD(GOTO_LINE2_POS0);
					strLCD(buff);
				}
			
				if(recv_flag)
				{
					/* PC side replied with the item's details: parse
					 * them, add the item's cost to the running total,
					 * and refresh the item/total display. */
					U0_recv();
					totalamt+=cost;
					item_details();
				}
				if(delete_flag)
				{
					delete_flag=0;
					if(totalamt==0)
					{
						/* Nothing in the cart yet - nothing to delete. */
						cmdLCD(CLEAR_LCD);
						strLCD("No items to");
						cmdLCD(GOTO_LINE2_POS0);
						strLCD("delete Add items");
						delay_s(2);
						strLCD("Place card and");
						cmdLCD(GOTO_LINE2_POS0);
						strLCD("Add items");
					}
					else
					{
						/* Enter a sub-loop: wait for either a card scan
						 * (to delete that item) or another Entry press
						 * (to go back to adding items). */
						cmdLCD(CLEAR_LCD);
						strLCD("Place card to");
						cmdLCD(GOTO_LINE2_POS0);
						strLCD("delete");
						while(1)
						{	
							if(scan_flag)
							{
								//scan_flag=0;
								/* Ask the PC side to remove one unit of
								 * this item from the cart. */
								snprintf(rfid,sizeof(rfid),"D%s$",buff);
								//U0_Txstring(rfid);
								//U0_TxByte('\0');
								U0_send(rfid);
								while(recv_flag==0);
								if(strcmp(rx_buff,"NO item")==0)
								{
									/* This card wasn't in the cart. */
									recv_flag=0;
									cmdLCD(CLEAR_LCD);
									strLCD("No such item to");
									cmdLCD(GOTO_LINE2_POS0);
									strLCD("delete Add item");
									delay_s(2);
									if(totalamt==0)
									{	
									cmdLCD(CLEAR_LCD);
									strLCD("Press Entry key");
									cmdLCD(GOTO_LINE2_POS0);
									strLCD("and add items");
									}
									else
									{
										cmdLCD(CLEAR_LCD);
										strLCD("TAmt:");
										s32LCD(totalamt);
										cmdLCD(GOTO_LINE2_POS0);
									}
								}
								else
								{
									/* Item removed successfully: update
									 * the running total and confirm on
									 * the LCD. */
									U0_recv();
									totalamt-=cost;	
									cmdLCD(CLEAR_LCD);
									strLCD("one ");
									strLCD(item_name);
									cmdLCD(GOTO_LINE2_POS0);
									strLCD(" item deleted");
									delay_s(2);
									item_details();
								}
							}
							else if(entry_flag==1)
							{
								/* User pressed Entry again: leave the
								 * delete sub-loop and go back to
								 * scanning items to add. */
								entry_flag=0;
								cmdLCD(CLEAR_LCD);
								if(totalamt>0)
								{
								strLCD("totalamt:");
								s32LCD(totalamt);
								cmdLCD(GOTO_LINE2_POS0);
								strLCD("Add MOre items");
								
								}
								else
								{
								strLCD("Place card and");
								cmdLCD(GOTO_LINE2_POS0);
								strLCD("Add items");
								}
								break;
							}
						}
					}
				}
				if(exit_flag)
				{
					/* Checkout requested: offer a payment method and
					 * loop until payment succeeds, is cancelled, or the
					 * user enters an invalid option (re-prompt via
					 * "goto Again"). */
					exit_flag=0;
					while(1)
					{
						Again:cmdLCD(CLEAR_LCD);
						strLCD("Select payment");
						cmdLCD(GOTO_LINE2_POS0);
						strLCD("     Method");
						delay_s(2);
						if(retry==0)
						{
							/* First attempt: only Cash/Card offered. */
							cmdLCD(CLEAR_LCD);
							strLCD("1.Cash  2.Card");
							op=keyscan();
							switch(op)
							{
								case '1':cmdLCD(CLEAR_LCD);
										cash_payment();
										break;
								case '2':card_payment(); 
										break;
								default:cmdLCD(CLEAR_LCD);
										strLCD("Select Valid");
										cmdLCD(GOTO_LINE2_POS0);
										strLCD("     Option");
										delay_s(2);
										goto Again;
							}
							if(retry==0)
							{
								/* Payment succeeded: end this shopping
								 * session and return to the idle prompt. */
								cmdLCD(CLEAR_LCD);
								strLCD("PRESS Entry Key");
								cmdLCD(GOTO_LINE2_POS0);
								strLCD("To Begin Shopping");
								break;
							}
						}
						else 
						{
							/* A previous payment attempt failed: also
							 * offer a Cancel option. */
							cmdLCD(CLEAR_LCD);
							strLCD("1.Cash  2.Card");
							cmdLCD(GOTO_LINE2_POS0);
							strLCD("3.Cancel");
							op=keyscan();
							switch(op)
							{
								case '1':cmdLCD(CLEAR_LCD);
										cash_payment();
										 break;
								case '2':card_payment(); 
										 break;
								case '3':U0_send("C$");
										 retry=0;
										 done=1;
										 totalamt=0;
										 cmdLCD(CLEAR_LCD);
										 strLCD("CANCELED BUYING!");
										 cmdLCD(GOTO_LINE2_POS0);
										 delay_s(2);
										 break;
								default:cmdLCD(CLEAR_LCD);
										strLCD("Select Valid");
										cmdLCD(GOTO_LINE2_POS0);
										strLCD("     Option");
										delay_s(2);
										goto Again;
							}
							if(retry==0)
							{
								/* Payment succeeded or purchase was
								 * cancelled: end this session. */
								cmdLCD(CLEAR_LCD);
								strLCD("PRESS Entry Key");
								cmdLCD(GOTO_LINE2_POS0);
								strLCD("To Begin Shopping");
								break;
							}
						}
					}
				}
				if(done==1)
				{
					/* Transaction complete: leave the item-scanning
					 * loop and go back to waiting for the next Entry
					 * button press. */
					done=0;
					break;
				}
			}
		}
		//cmdLCD(CLEAR_LCD);
		//strLCD("smart cart RFID");
		//delay_ms(2000);
	}
}
