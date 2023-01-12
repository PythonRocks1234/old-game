// todo: offline progress (use p += 60 * time_passed * deltaP * multi) DONE
// todo: buy 1/max DONE
// todo: long -> unsigned long (maybe unsigned long long) DONE
// todo: milestones + tab for milestones
// todo: autobuy
// todo: fix blinkoing when too many chars on screen

#include <nds.h>
#include <fat.h>
#include <stdio.h>
#include <math.h>
#include <time.h>

typedef enum {
	FLAGS_MAINLOOP,
	FLAGS_CONFIRM,
	FLAGS_MILESTONES
} FlagStates;

typedef enum {
	FLAGS_ONE,
	FLAGS_MAX
} BuyStates;

long double root(long double number) {
	return 2 * pow(number, 0.15);
}

int bit_enable_disable(bool cond, int modify) {
	return ((cond) ? (modify | (1 << 15)) : (modify & ((1 << 15) - 1)));
}

int bit_flip(bool cond) {
	if (cond) { 
		return ARGB16(1, 7, 21, 11);
	} else {
		return ARGB16(1, 15, 5, 0);
	}
}

double format_for_print(long long unsigned z) {
	return pow(10, (log10(z)-(int)(log10(z))));
}

int main(void)  {
	// enable printing
	touchPosition touch;

	PrintConsole topScreen;
	PrintConsole bottomScreen;
	
	videoSetMode(MODE_0_2D);
	videoSetModeSub(MODE_3_2D);

	consoleInit(&topScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 2, 0, true, true);
	consoleInit(&bottomScreen, 0, BgType_Text4bpp, BgSize_T_256x256, 2, 0, false, true);

	// int mainId = bgInit(3, BgType_Bmp16, BgSize_B16_256x256, 1, 0);
	int subId = bgInitSub(3, BgType_Bmp16, BgSize_B16_256x256, 1, 0);
	// u16* mainPtr = bgGetGfxPtr(mainId);
	u16* subPtr = bgGetGfxPtr(subId);

	time_t unixTime = time(NULL);
	// unix timestamp in gmt+0
	long long unsigned int llx = unixTime;

	// save attempt
	FILE* savefile;
	struct VariablesToSave {
		long long unsigned p;
		long long unsigned deltaP;
		long long unsigned deltaPcost;
		long long unsigned tau;
		long double multi;
		long long unsigned prevtau;
		long long unsigned int time;
	} SaveFile;

	if (fatInitDefault()) {
		savefile = fopen("fat:/first_game_save1.sav","rb");

		if (savefile == NULL) {
			fclose(savefile);
			SaveFile.p = 0;
			SaveFile.deltaP = 0;
			SaveFile.deltaPcost = 0;
			SaveFile.tau = 0;
			SaveFile.multi = 1;
			SaveFile.prevtau = 0;
			SaveFile.time = llx;
			savefile = fopen("fat:/first_game_save1.sav","wb");
			fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
			fclose(savefile);
		} else {
			fclose(savefile);
		}
		
		savefile = fopen("fat:/first_game_save1.sav","rb");
		fread(&SaveFile,1,sizeof(SaveFile),savefile);
		fclose(savefile);
	} else {
		exit(1);
	}

	for (int y = 80; y < 100; y++) {
		for (int x = 60; x < 120; x++) {
			subPtr[x + (y << 8)] = ARGB16(0, 3, 26, 5);
		}
		for (int x = 140; x < 190; x++) {
			subPtr[x + (y << 8)] = ARGB16(0, 26, 5, 3);
		}
	}
	
	for (int y = 102; y < 119; y++) {
		for (int x = 0; x < 81; x++) {
			subPtr[x + (y << 8)] = ARGB16(1, 11, 18, 16);
		}
		for (int x = 208; x < 256; x++) {
			subPtr[x + (y << 8)] = ARGB16(1, 10, 16, 13);
		}
	}

	for (int y = 119; y < 152; y++) {
		for (int x = 0; x < 256; x++) {
			subPtr[x + (y << 8)] = ARGB16(1, 7, 21, 11);
		}
	}

	for (int y = 152; y < 192; y++) {
		for (int x = 0; x < 256; x++) {
			subPtr[x + (y << 8)] = ARGB16(0, 0, 15, 15);
		}
	}

	// init vars
	long long unsigned p = SaveFile.p;
	long long unsigned deltaP = SaveFile.deltaP;
	long long unsigned deltaPcost = SaveFile.deltaPcost;
	long long unsigned tau = SaveFile.tau;
	long double multi = SaveFile.multi;
	long long unsigned prevtau = SaveFile.prevtau;
	long long unsigned int time = SaveFile.time;
	
	int time_passed = unixTime - time;
	uint frames = 0;

	FlagStates current_state = FLAGS_MAINLOOP;
	BuyStates current_buy = FLAGS_ONE;

	long long earned = 60 * time_passed * deltaP * multi;
	p += earned;

	while(1) {
		consoleSelect(&topScreen);
		long long unsigned temp = deltaP * multi;
		iprintf("p = %lld\n", p);
		iprintf("tau = %lld\n", tau);
		iprintf("previous tau = %lld\n", prevtau);
		// no i because i = integer and we are printing a double
		printf("multi = %.5Lf\n", multi);
		iprintf("multi = 2 * previous tau^0.15\n");
		iprintf("delta p = %lld\n", deltaP);
		iprintf("cost to increase delta p =\n%lld\n", deltaPcost);
		iprintf("delta p * multi = %lld\n", temp);
		iprintf("each frame p += delta p * multi\n");
		iprintf("A to increase delta p\n");
		iprintf("B to publish\n");
		iprintf("L/R to buy 1/max\n");
		iprintf((p >= deltaPcost) ? "You can buy delta p now\n" : "You cannot buy delta p now\n");
		iprintf((tau > prevtau) ? "You can publish now" : "You cannot publish now");
		if (tau > prevtau) {
			printf("\nMulti = %.5Lf after publish", root(tau));
		}
		if (frames < 180) { // 30 seconds
			iprintf("\ntime difference: %i\n", time_passed);
			iprintf("Earned %lld p (offline)", earned);
		}
		consoleSelect(&bottomScreen);
		printf("\x1b[0;0H%lld p (%.4fe%i)\n", p, format_for_print(p), (p == 0) ? 0 : (int) log10(p));
		printf("%lld tau (%.4fe%i)\n", tau, format_for_print(tau), (tau == 0) ? 0 : (int) log10(tau));
		printf("%lld prevtau (%.4fe%i)\n", prevtau, format_for_print(prevtau), (prevtau == 0) ? 0 : (int) log10(prevtau));
		iprintf("p <- p + delta p * multi\n\n");
		iprintf("Press SELECT to save\n");
		iprintf("Press START to save + exit\n");
		if (current_state == FLAGS_MAINLOOP || current_state == FLAGS_CONFIRM) {
			long long unsigned dispdeltaP = (current_buy == FLAGS_ONE) ? deltaP+1 : 0;
			long long unsigned dispdeltaPcost = (current_buy == FLAGS_ONE) ? deltaPcost : 0;
			if (dispdeltaP == 0) {
				if (p < deltaPcost) {
					dispdeltaP = deltaP+1;
					dispdeltaPcost = deltaPcost;
				} else {
					long long unsigned p_copy = p;
					long long unsigned deltaP_copy = deltaP;
					long long unsigned deltaPcost_copy = deltaPcost;
					do {
						deltaP_copy++;
						p_copy -= deltaPcost_copy;
						deltaPcost_copy *= 1.2;
						deltaPcost_copy = (deltaPcost_copy == 0) ? 5 : deltaPcost_copy;
					} while (p_copy >= deltaPcost_copy);
					dispdeltaP = deltaP_copy;
					dispdeltaPcost = p - p_copy;
				}
			}
			iprintf("\x1b[15;0Hdelta p = %lld -> %lld (%lld p)\n", deltaP, dispdeltaP, dispdeltaPcost);
			iprintf("delta p * multi = %lld -> ", temp);
			temp = (current_buy == FLAGS_ONE) ? (deltaP+1) * multi : dispdeltaP * multi;
			iprintf("%lld\n", temp);
			if (current_state == FLAGS_CONFIRM) {
				iprintf("\x1b[8;8HConfirm publish?");
				iprintf("\x1b[11;8HYes (Y)");
				iprintf("\x1b[11;18HNo (X)");
			}
		}
		iprintf("\x1b[13;26Hx");
		iprintf((current_buy == FLAGS_ONE) ? "1" : "MAX");
		iprintf("\x1b[13;0H");
		iprintf((current_state == FLAGS_MILESTONES) ? "Milestones" : "Upgrades");

		for (int y = 102; y < 119; y++) {
			for (int x = 0; x < 81; x++) {
				subPtr[x + (y << 8)] = ARGB16(1, 11, 18, 16);
			}
			for (int x = 208; x < 256; x++) {
				subPtr[x + (y << 8)] = ARGB16(1, 10, 16, 13);
			}
		}

		for (int y = 80; y < 100; y++) {
			for (int x = 60; x < 120; x++) {
				subPtr[x + (y << 8)] = bit_enable_disable((current_state == FLAGS_CONFIRM), subPtr[x + (y << 8)]);
			}
			for (int x = 140; x < 190; x++) {
				subPtr[x + (y << 8)] = bit_enable_disable((current_state == FLAGS_CONFIRM), subPtr[x + (y << 8)]);
			}
		}
		for (int y = 119; y < 152; y++) {
			for (int x = 0; x < 256; x++) {
				subPtr[x + (y << 8)] = bit_flip(p >= deltaPcost);
				subPtr[x + (y << 8)] = bit_enable_disable(current_state == FLAGS_CONFIRM || current_state == FLAGS_MAINLOOP, subPtr[x + (y << 8)]);
			}
		}
		for (int y = 152; y < 192; y++) {
			for (int x = 0; x < 256; x++) {
				subPtr[x + (y << 8)] = bit_enable_disable((tau > prevtau && (current_state == FLAGS_CONFIRM || current_state == FLAGS_MAINLOOP)), subPtr[x + (y << 8)]);
			}
		}
		if (tau > prevtau && (current_state == FLAGS_CONFIRM || current_state == FLAGS_MAINLOOP)) {
			iprintf("\x1b[20;0Htau = %lld -> %lld\n", prevtau, tau);
			printf("\x1b[21;0Hmulti = %.5Lf -> %.5Lf\n", multi, root(tau));
		}

		scanKeys();
		int keys = keysDown();

		if (keys & KEY_START) {
			// current_state in (FLAGS_MAINLOOP, FLAGS_MILESTONES) // but in c
			if (current_state == FLAGS_MAINLOOP || current_state == FLAGS_MILESTONES) {
				SaveFile.p = p;
				SaveFile.deltaP = deltaP;
				SaveFile.deltaPcost = deltaPcost;
				SaveFile.tau = tau;
				SaveFile.multi = multi;
				SaveFile.prevtau = prevtau;
				SaveFile.time = unixTime + frames / 60;
				savefile = fopen("fat:/first_game_save1.sav","wb");
				fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
				fclose(savefile);
				exit(0);
			}
		}

		if (keys & KEY_A) {
			if (p >= deltaPcost && current_state == FLAGS_MAINLOOP) {
				if (current_buy == FLAGS_ONE) {
					deltaP++;
					p -= deltaPcost;
					deltaPcost *= 1.2;
					deltaPcost = (deltaPcost == 0) ? 5 : deltaPcost;
				} else {
					do {
						deltaP++;
						p -= deltaPcost;
						deltaPcost *= 1.2;
						deltaPcost = (deltaPcost == 0) ? 5 : deltaPcost;
					} while (p >= deltaPcost);
				}
			}
		}

		if (keys & KEY_B) {
			if (tau > prevtau && current_state == FLAGS_MAINLOOP) {
				current_state = FLAGS_CONFIRM;
			}
		}

		if (keys & KEY_X) {
			if (current_state == FLAGS_CONFIRM) {
				current_state = FLAGS_MAINLOOP;
			}
		}

		if (keys & KEY_Y) {
			if (current_state == FLAGS_CONFIRM) {
				deltaP = 0;
				deltaPcost = 0;
				p = 0;
				prevtau = tau;
				tau = 0;
				multi = root(prevtau);
				current_state = FLAGS_MAINLOOP;
			}
		}

		if ((keys & KEY_L) || (keys & KEY_R)) {
			if (current_buy == FLAGS_ONE) {
				current_buy = FLAGS_MAX;
			} else {
				current_buy = FLAGS_ONE;
			}
		}

		if (keys & KEY_TOUCH) {
			touchRead(&touch);
			if (touch.py > 101 && touch.py < 119) {
				if (touch.px > 207 && touch.px < 256) {
					if (current_buy == FLAGS_ONE) {
						current_buy = FLAGS_MAX;
					} else {
						current_buy = FLAGS_ONE;
					}
				}
				if (touch.px > -1 && touch.px < 81) {
					if (current_state == FLAGS_MAINLOOP) {
						current_state = FLAGS_MILESTONES;
					} else if (current_state == FLAGS_MILESTONES) {
						current_state = FLAGS_MAINLOOP;
					}
				}
			}
			if (current_state == FLAGS_MAINLOOP) {
				if (touch.py > 118 && touch.py < 152) {
					if (p >= deltaPcost) {
						if (current_buy == FLAGS_ONE) {
							deltaP++;
							p -= deltaPcost;
							deltaPcost *= 1.2;
							deltaPcost = (deltaPcost == 0) ? 5 : deltaPcost;
						} else {
							do {
								deltaP++;
								p -= deltaPcost;
								deltaPcost *= 1.2;
								deltaPcost = (deltaPcost == 0) ? 5 : deltaPcost;
							} while (p >= deltaPcost);
						}
					}
				}

				if (touch.py > 151 && touch.py < 192) {
					if (tau > prevtau) {
						current_state = FLAGS_CONFIRM;
					}
				}
			}
			if (current_state == FLAGS_CONFIRM) {
				if (touch.py > 79 && touch.py < 100) {
					if (touch.px > 59 && touch.px < 120) {
						deltaP = 0;
						deltaPcost = 0;
						p = 0;
						prevtau = tau;
						tau = 0;
						multi = root(prevtau);
						current_state = FLAGS_MAINLOOP;
					}
					if (touch.px > 139 && touch.px < 190) {
						current_state = FLAGS_MAINLOOP;
					}
				}
			}
		}

		if (keys & KEY_SELECT) {
			if (current_state == FLAGS_MAINLOOP) {
				SaveFile.p = p;
				SaveFile.deltaP = deltaP;
				SaveFile.deltaPcost = deltaPcost;
				SaveFile.tau = tau;
				SaveFile.multi = multi;
				SaveFile.prevtau = prevtau;
				SaveFile.time = unixTime + frames / 60;
				savefile = fopen("fat:/first_game_save1.sav","wb");
				fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
				fclose(savefile);
			}
		}

		p += (deltaP * multi);

		// max function not builtin
		if (p > tau) {
			tau = p;
		}

		frames++;

		if (frames % 60 == 0) {
			SaveFile.time = unixTime + frames / 60;
			savefile = fopen("fat:/first_game_save1.sav","wb");
			fwrite(&SaveFile,1,sizeof(SaveFile),savefile);
			fclose(savefile);
		}

		// wait for next frame, clear screen
		swiWaitForVBlank();
		consoleSelect(&topScreen);
		consoleClear();
		consoleSelect(&bottomScreen);
		consoleClear();
	}
	return 0;
}