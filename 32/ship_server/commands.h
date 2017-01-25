void CommandED(PSO_CLIENT* client)
{
	switch (client->decryptbuf[0x03])
	{
		case 0x01:
			// Options
			*(unsigned *) &client->character.options[0] = *(unsigned *) &client->decryptbuf[0x08];
			break;
		case 0x02:
			// Symbol Chats
			memcpy (&client->character.symbol_chats, &client->decryptbuf[0x08], 1248);
			break;
		case 0x03:
			// Shortcuts
			memcpy (&client->character.shortcuts, &client->decryptbuf[0x08], 2624);
			break;
		case 0x04:
			// Global Key Config
			memcpy (&client->character.keyConfigGlobal, &client->decryptbuf[0x08], 364);
			break;
		case 0x05:
			// Global Joystick Config
			memcpy (&client->character.joyConfigGlobal, &client->decryptbuf[0x08], 56);
			break;
		case 0x06:
			// Technique Config
			memcpy (&client->character.techConfig, &client->decryptbuf[0x08], 40);
			break;
		case 0x07:
			// Character Key Config
			memcpy (&client->character.keyConfig, &client->decryptbuf[0x08], 232);
			break;
		case 0x08:
			// C-Rank and Battle Config
			//memcpy (&client->character.challengeData, &client->decryptbuf[0x08], 320);
			break;
	}
}

void Command40(PSO_CLIENT* client, PSO_SERVER* ship)
{
	// Guild Card Search

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x01;
	*(unsigned *) &ship->encryptbuf[0x02] = *(unsigned *) &client->decryptbuf[0x10];
	*(unsigned *) &ship->encryptbuf[0x06] = *(unsigned *) &client->guildcard;
	*(unsigned *) &ship->encryptbuf[0x0A] = serverID;
	*(unsigned *) &ship->encryptbuf[0x0E] = client->character.teamID;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x21);
}

void Command09(PSO_CLIENT* client)
{
	QUEST* q;
	PSO_CLIENT* c;
	LOBBY* l;
	unsigned lobbyNum, Packet11_Length, ch;
	char lb[10];
	int num_hours, num_minutes;

	switch ( client->decryptbuf[0x0F] )
	{
	case 0x00:
		// Team info
		if ( client->lobbyNum < 0x10 )
		{
			if ((!client->block) || (client->block > serverBlocks))
			{
				initialize_connection (client);
				return;
			}

			lobbyNum = *(unsigned *) &client->decryptbuf[0x0C];

			if ((lobbyNum < 0x10) || (lobbyNum >= 16+SHIP_COMPILED_MAX_GAMES))
			{
				initialize_connection (client);
				return;
			}

			l = &blocks[client->block - 1]->lobbies[lobbyNum];
			memset (&PacketData, 0, 0x10);
			PacketData[0x02] = 0x11;
			PacketData[0x0A] = 0x20;
			PacketData[0x0C] = 0x20;
			PacketData[0x0E] = 0x20;
			if (l->in_use)
			{
				Packet11_Length = 0x10;
				if ((client->team_info_request != lobbyNum) || (client->team_info_flag == 0))
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 1;
					for (ch=0;ch<4;ch++)
						if ((l->slot_use[ch]) && (l->clients[ch]))
						{
							c = l->clients[ch];
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &c->character.name[0]);
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4C;
							PacketData[Packet11_Length++] = 0x00;
							_itoa (l->clients[ch]->character.level + 1, &lb[0], 10);
							wstrcpy_char(&PacketData[Packet11_Length], &lb[0]);
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							switch (c->character._class)
							{
							case CLASS_HUMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "HUmar");
								break;
							case CLASS_HUNEWEARL:
								wstrcpy_char (&PacketData[Packet11_Length], "HUnewearl");
								break;
							case CLASS_HUCAST:
								wstrcpy_char (&PacketData[Packet11_Length], "HUcast");
								break;
							case CLASS_HUCASEAL:
								wstrcpy_char (&PacketData[Packet11_Length], "HUcaseal");
								break;
							case CLASS_RAMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "RAmar");
								break;
							case CLASS_RACAST:
								wstrcpy_char (&PacketData[Packet11_Length], "RAcast");
								break;
							case CLASS_RACASEAL:
								wstrcpy_char (&PacketData[Packet11_Length], "RAcaseal");
								break;
							case CLASS_RAMARL:
								wstrcpy_char (&PacketData[Packet11_Length], "RAmarl");
								break;
							case CLASS_FONEWM:
								wstrcpy_char (&PacketData[Packet11_Length], "FOnewm");
								break;
							case CLASS_FONEWEARL:
								wstrcpy_char (&PacketData[Packet11_Length], "FOnewearl");
								break;
							case CLASS_FOMARL:
								wstrcpy_char (&PacketData[Packet11_Length], "FOmarl");
								break;
							case CLASS_FOMAR:
								wstrcpy_char (&PacketData[Packet11_Length], "FOmar");
								break;
							default:
								wstrcpy_char (&PacketData[Packet11_Length], "Unknown");
								break;
							}
							Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x20;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x4A;
							PacketData[Packet11_Length++] = 0x00;
							PacketData[Packet11_Length++] = 0x0A;
							PacketData[Packet11_Length++] = 0x00;
						}
				}
				else
				{
					client->team_info_request = lobbyNum;
					client->team_info_flag = 0;
					wstrcpy_char (&PacketData[Packet11_Length], "Time : ");
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					num_minutes  = ((unsigned) servertime - l->start_time ) / 60L;
					num_hours    = num_minutes / 60L;
					num_minutes %= 60;
					_itoa (num_hours,&lb[0], 10);
					wstrcpy_char (&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					PacketData[Packet11_Length++] = 0x3A;
					PacketData[Packet11_Length++] = 0x00;
					_itoa (num_minutes,&lb[0], 10);
					if (num_minutes < 10)
					{
						lb[1] = lb[0];
						lb[0] = 0x30;
						lb[2] = 0x00;
					}
					wstrcpy_char (&PacketData[Packet11_Length], &lb[0]);
					Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);					
					PacketData[Packet11_Length++] = 0x0A;
					PacketData[Packet11_Length++] = 0x00;
					if (l->quest_loaded)
					{
						wstrcpy_char (&PacketData[Packet11_Length], "Quest : ");
						Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
						PacketData[Packet11_Length++] = 0x0A;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						PacketData[Packet11_Length++] = 0x20;
						PacketData[Packet11_Length++] = 0x00;
						q = &quests[l->quest_loaded - 1];
						if ((client->character.lang < 10) && (q->ql[client->character.lang]))
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &q->ql[client->character.lang]->qname[0]);
						else
							wstrcpy((unsigned short*) &PacketData[Packet11_Length], (unsigned short*) &q->ql[0]->qname[0]);
						Packet11_Length += wstrlen ((unsigned short*) &PacketData[Packet11_Length]);
					}
				}
			}
			else
			{
				wstrcpy_char (&PacketData[0x10], "Game no longer active.");
				Packet11_Length = 0x10 + (strlen ("Game no longer active.") * 2);
			}
			PacketData[Packet11_Length++] = 0x00;
			PacketData[Packet11_Length++] = 0x00;
			*(unsigned short*) &PacketData[0x00] = (unsigned short) Packet11_Length;
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketData[0], Packet11_Length );
		}
		break;
	case 0x0F:
		// Quest info
		if ( ( client->lobbyNum > 0x0F ) && ( client->decryptbuf[0x0B] <= numQuests ) )
		{
			q = &quests[client->decryptbuf[0x0B] - 1];
			memset (&PacketData[0x00], 0, 8);
			PacketData[0x00] = 0x48;
			PacketData[0x01] = 0x02;
			PacketData[0x02] = 0xA3;
			if ((client->character.lang < 10) && (q->ql[client->character.lang]))
				memcpy (&PacketData[0x08], &q->ql[client->character.lang]->qdetails[0], 0x200 );
			else
				memcpy (&PacketData[0x08], &q->ql[0]->qdetails[0], 0x200 );
			cipher_ptr = &client->server_cipher;
			encryptcopy (client, &PacketData[0], 0x248 );
		}
		break;
	default:
		break;
	}
}

void Command10(unsigned blockServer, PSO_CLIENT* client)
{
	unsigned char select_type, selected;
	unsigned full_select, ch, ch2, failed_to_join, lobbyNum, password_match, oldIndex;
	LOBBY* l;
	unsigned short* p;
	unsigned short* c;
	unsigned short fqs, barule;
	unsigned qm_length, qa, nr;
	unsigned char* qmap;
	QUEST* q;
	char quest_num[16];
	unsigned qn;
	int do_quest;
	unsigned quest_flag;

	if (client->guildcard)
	{
		select_type = (unsigned char) client->decryptbuf[0x0F];
		selected = (unsigned char) client->decryptbuf[0x0C];
		full_select = *(unsigned *) &client->decryptbuf[0x0C];

		switch (select_type)
		{
		case 0x00:
			if ( ( blockServer ) && ( client->lobbyNum < 0x10 ) )
			{
				// Team

				if ((!client->block) || (client->block > serverBlocks))
				{
					initialize_connection (client);
					return;
				}

				lobbyNum = *(unsigned *) &client->decryptbuf[0x0C];

				if ((lobbyNum < 0x10) || (lobbyNum >= 16+SHIP_COMPILED_MAX_GAMES))
				{
					initialize_connection (client);
					return;
				}

				failed_to_join = 0;
				l = &blocks[client->block - 1]->lobbies[lobbyNum];

				if ((!client->isgm) && (!isLocalGM(client->guildcard)))
				{
					switch (l->episode)
					{
					case 0x01:
						if ((l->difficulty == 0x01) && (client->character.level < 19))
						{
							Send01 ("Episode I\n\nYou must be level\n20 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 49))
							{
								Send01 ("Episode I\n\nYou must be level\n50 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 89))
								{
									Send01 ("Episode I\n\nYou must be level\n90 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					case 0x02:
						if ((l->difficulty == 0x01) && (client->character.level < 29))
						{
							Send01 ("Episode II\n\nYou must be level\n30 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 59))
							{
								Send01 ("Episode II\n\nYou must be level\n60 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 99))
								{
									Send01 ("Episode II\n\nYou must be level\n100 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					case 0x03:
						if ((l->difficulty == 0x01) && (client->character.level < 39))
						{
							Send01 ("Episode IV\n\nYou must be level\n40 or higher\nto play on the\nhard difficulty.", client);
							failed_to_join = 1;
						}
						else
							if ((l->difficulty == 0x02) && (client->character.level < 69))
							{
								Send01 ("Episode IV\n\nYou must be level\n70 or higher\nto play on the\nvery hard\ndifficulty.", client);
								failed_to_join = 1;
							}
							else
								if ((l->difficulty == 0x03) && (client->character.level < 109))
								{
									Send01 ("Episode IV\n\nYou must be level\n110 or higher\nto play on the\nultimate\ndifficulty.", client);
									failed_to_join = 1;
								}
								break;
					}
				}


				if ((!l->in_use) && (!failed_to_join))
				{
					Send01 ("Game no longer active.", client);
					failed_to_join = 1;
				}

				if ((l->lobbyCount == 4)  && (!failed_to_join))
				{
					Send01 ("Game is full", client);
					failed_to_join = 1;
				}

				if ((l->quest_in_progress) && (!failed_to_join))
				{
					Send01 ("Quest already in progress.", client);
					failed_to_join = 1;
				}

				if ((l->oneperson) && (!failed_to_join))
				{
					Send01 ("Cannot join a one\nperson game.", client);
					failed_to_join = 1;
				}

				if (((l->gamePassword[0x00] != 0x00) || (l->gamePassword[0x01] != 0x00)) &&
					(!failed_to_join))
				{
					password_match = 1;
					p = (unsigned short*) &l->gamePassword[0x00];
					c = (unsigned short*) &client->decryptbuf[0x10];
					while (*p != 0x00)
					{
						if (*p != *c)
							password_match = 0;
						p++;
						c++;
					}
					if ((password_match == 0) && (client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
					{
						Send01 ("Incorrect password.", client);
						failed_to_join = 1;
					}
				}

				if (!failed_to_join)
				{
					for (ch=0;ch<4;ch++)
					{
						if ((l->slot_use[ch]) && (l->clients[ch]))
						{
							if (l->clients[ch]->bursting == 1)
							{
								Send01 ("Player is bursting.\nPlease wait a\nmoment.", client);
								failed_to_join = 1;
							}
							else
								if ((l->inpquest) && (!l->clients[ch]->hasquest))
								{
									Send01 ("Player is loading\nquest.\nPlease wait a\nmoment.", client);
									failed_to_join = 1;
								}
						}
					}
				}

				if ((l->inpquest) && (!failed_to_join))
				{
					// Check if player qualifies to join Government quest...
					q = &quests[l->quest_loaded - 1];
					memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
					dp[4] = 0;
					qn = (unsigned) atoi ( &dp[0] );
					switch (l->episode)
					{
					case 0x01:
						qn -= 401;
						qn <<= 1;
						qn += 0x1F3;
						for (ch2=0x1F5;ch2<=qn;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x02:
						qn -= 451;
						qn <<= 1;
						qn += 0x211;
						for (ch2=0x213;ch2<=qn;ch2+=2)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					case 0x03:
						qn -= 701;
						qn += 0x2BC;
						for (ch2=0x2BD;ch2<=qn;ch2++)
							if (!qflag(&client->character.quest_data1[0], ch2, l->difficulty))
								failed_to_join = 1;
						break;
					}
					if (failed_to_join)
					{
						if ((client->isgm == 0) && (isLocalGM(client->guildcard) == 0))
							Send01 ("You must progress\nfurther in the\ngame before you\ncan join this\nquest.", client);
						else
							failed_to_join = 0;
					}
				}

				if (failed_to_join == 0)
				{
					removeClientFromLobby (client);
					client->lobbyNum = lobbyNum + 1;
					client->lobby = (void*) l;
					Send64 (client);
					memset (&client->encryptbuf[0x00], 0, 0x0C);
					client->encryptbuf[0x00] = 0x0C;
					client->encryptbuf[0x02] = 0x60;
					client->encryptbuf[0x08] = 0xDD;
					client->encryptbuf[0x09] = 0x03;
					client->encryptbuf[0x0A] = (unsigned char) experience_rate;
					cipher_ptr = &client->server_cipher;
					encryptcopy (client, &client->encryptbuf[0x00], 0x0C);
					UpdateGameItem (client);
				}
			}
			break;
		case 0x0F:
			// Quest selection
			if ( ( blockServer ) && ( client->lobbyNum > 0x0F ) )
			{
				if (!client->lobby)
					break;

				l = (LOBBY*) client->lobby;

				if ( client->decryptbuf[0x0B] == 0 )
				{
					if ( client->decryptbuf[0x0C] < 11 )
						SendA2 ( l->episode, l->oneperson, client->decryptbuf[0x0C], client->decryptbuf[0x0A], client );
				}
				else
				{
					if ( l->leader == client->clientID )
					{
						if ( l->quest_loaded == 0 )
						{
							if ( client->decryptbuf[0x0B] <= numQuests )
							{
								q = &quests[client->decryptbuf[0x0B] - 1];

								do_quest = 1;

								// Check "One-Person" quest ability to repeat...

								if ( ( l->oneperson ) && ( l->episode == 0x01 ) )
								{
									memcpy (&quest_num[0], &q->ql[0]->qdata[49], 3);
									quest_num[4] = 0;
									qn = atoi (&quest_num[0]);
									quest_flag = 0x63 + (qn << 1);
									if (qflag(&client->character.quest_data1[0], quest_flag, l->difficulty))
									{
										if (!qflag_ep1solo(&client->character.quest_data1[0], l->difficulty))
											do_quest = 0;
									}
									if ( !do_quest )
										Send01 ("Please clear\nthe remaining\nquests before\nredoing this one.", client);
								}

								// Check party Government quest qualification.  (Teamwork?!)

								if ( client->decryptbuf[0x0A] )
								{
									memcpy (&dp[0], &q->ql[0]->qdata[0x31], 3);
									dp[4] = 0;
									qn = (unsigned) atoi ( &dp[0] );
									switch (l->episode)
									{
									case 0x01:
										qn -= 401;
										qn <<= 1;
										qn += 0x1F3;
										for (ch2=0x1F5;ch2<=qn;ch2+=2)
											for (ch=0;ch<4;ch++)
												if ((l->clients[ch]) && (!qflag(&l->clients[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x02:
										qn -= 451;
										qn <<= 1;
										qn += 0x211;
										for (ch2=0x213;ch2<=qn;ch2+=2)
											for (ch=0;ch<4;ch++)
												if ((l->clients[ch]) && (!qflag(&l->clients[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									case 0x03:
										qn -= 701;
										qn += 0x2BC;
										for (ch2=0x2BD;ch2<=qn;ch2++)
											for (ch=0;ch<4;ch++)
												if ((l->clients[ch]) && (!qflag(&l->clients[ch]->character.quest_data1[0], ch2, l->difficulty)))
													do_quest = 0;
										break;
									}
									if (!do_quest)
										Send01 ("The party no longer\nqualifies to\nstart this quest.", client);
								}

								if ( do_quest )
								{
									ch2 = 0;
									barule = 0;

									while (q->ql[0]->qname[ch2] != 0x00)
									{
										// Search for a number in the quest name to determine battle rule #
										if ((q->ql[0]->qname[ch2] >= 0x31) && (q->ql[0]->qname[ch2] <= 0x38))
										{
											barule = q->ql[0]->qname[ch2];
											break;
										}
										ch2++;
									}

									for (ch=0;ch<4;ch++)
										if ((l->slot_use[ch]) && (l->clients[ch]))
										{
											if ((l->battle) || (l->challenge))
											{
												// Copy character to backup buffer.
												if (l->clients[ch]->character_backup)
													free (l->clients[ch]->character_backup);
												l->clients[ch]->character_backup = malloc (sizeof (l->clients[ch]->character));
												memcpy ( l->clients[ch]->character_backup, &l->clients[ch]->character, sizeof (l->clients[ch]->character));

												l->battle_level = 0;

												switch ( barule )
												{
												case 0x31:
													// Rule #1
													l->clients[ch]->mode = 1;
													break;
												case 0x32:
													// Rule #2
													l->battle_level = 1;
													l->clients[ch]->mode = 3;
													break;
												case 0x33:
													// Rule #3
													l->battle_level = 5;
													l->clients[ch]->mode = 3;
													break;
												case 0x34:
													// Rule #4
													l->battle_level = 2;
													l->clients[ch]->mode = 3;
													l->meseta_boost = 1;
													break;
												case 0x35:
													// Rule #5
													l->clients[ch]->mode = 2;
													l->meseta_boost = 1;
													break;
												case 0x36:
													// Rule #6
													l->battle_level = 20;
													l->clients[ch]->mode = 3;
													break;
												case 0x37:
													// Rule #7
													l->battle_level = 1;
													l->clients[ch]->mode = 3;
													break;
												case 0x38:
													// Rule #8
													l->battle_level = 20;
													l->clients[ch]->mode = 3;
													break;
												default:
													write_log ("ship.log", "Unknown battle rule loaded...");
													l->clients[ch]->mode = 1;
													break;
												}

												switch (l->clients[ch]->mode)
												{
												case 0x02:
													// Delete all mags and meseta...
													for (ch2=0;ch2<l->clients[ch]->character.inventoryUse;ch2++)
													{
														if (l->clients[ch]->character.inventory[ch2].item.data[0] == 0x02)
															l->clients[ch]->character.inventory[ch2].in_use = 0;
													}
													CleanUpInventory (l->clients[ch]);
													l->clients[ch]->character.meseta = 0;
													break;
												case 0x03:
													// Wipe items and reset level.
													for (ch2=0;ch2<30;ch2++)
														l->clients[ch]->character.inventory[ch2].in_use = 0;
													CleanUpInventory (l->clients[ch]);
													l->clients[ch]->character.level = 0;
													l->clients[ch]->character.XP = 0;
													l->clients[ch]->character.ATP = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)];
													l->clients[ch]->character.MST = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)+2];
													l->clients[ch]->character.EVP = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)+4];
													l->clients[ch]->character.HP  = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)+6];
													l->clients[ch]->character.DFP = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)+8];
													l->clients[ch]->character.ATA = *(unsigned short*) &startingData[(l->clients[ch]->character._class*14)+10];
													if (l->battle_level > 1)
														SkipToLevel (l->battle_level - 1, l->clients[ch], 1);
													l->clients[ch]->character.meseta = 0;
												}
											}

											if ((l->clients[ch]->character.lang < 10) &&
												(q->ql[l->clients[ch]->character.lang]))
											{
												fqs = *(unsigned short*) &q->ql[l->clients[ch]->character.lang]->qdata[0];
												if (fqs % 8)
													fqs += ( 8 - ( fqs % 8 ) );
												cipher_ptr = &l->clients[ch]->server_cipher;
												encryptcopy (l->clients[ch], &q->ql[l->clients[ch]->character.lang]->qdata[0], fqs);
											}
											else
											{
												fqs = *(unsigned short*) &q->ql[0]->qdata[0];
												if (fqs % 8)
													fqs += ( 8 - ( fqs % 8 ) );
												cipher_ptr = &l->clients[ch]->server_cipher;
												encryptcopy (l->clients[ch], &q->ql[0]->qdata[0], fqs);
											}
											l->clients[ch]->bursting = 1;
											l->clients[ch]->sending_quest = client->decryptbuf[0x0B] - 1;
											l->clients[ch]->qpos = fqs;
										}
										if (!client->decryptbuf[0x0A])
											l->quest_in_progress = 1; // when a government quest, this won't be set
										else
											l->inpquest = 1;

										l->quest_loaded = client->decryptbuf[0x0B];

										// Time to load the map data...

										memset ( &l->mapData[0], 0, 0xB50 * sizeof (MAP_MONSTER) ); // Erase!
										l->mapIndex = 0;
										l->rareIndex = 0;
										for (ch=0;ch<0x20;ch++)
											l->rareData[ch] = 0xFF;

										qmap = q->mapdata;
										qm_length = *(unsigned*) qmap;
										qmap += 4;
										ch = 4;
										while ( ( qm_length - ch ) >= 80 )
										{
											oldIndex = l->mapIndex;
											qa = *(unsigned*) qmap; // Area
											qmap += 4;
											nr = *(unsigned*) qmap; // Number of monsters
											qmap += 4;
											if ( ( l->episode == 0x03 ) && ( qa > 5 ) )
												parse_map_data ( l, (MAP_MONSTER*) qmap, 1, nr );
											else
												if ( ( l->episode == 0x02 ) && ( qa > 15 ) )
													parse_map_data ( l, (MAP_MONSTER*) qmap, 1, nr );
												else
													parse_map_data ( l, (MAP_MONSTER*) qmap, 0, nr );
											qmap += ( nr * 72 );
											ch += ( ( nr * 72 ) + 8 );
											//debug ("loaded quest area %u, mid count %u, total mids: %u", qa, l->mapIndex - oldIndex, l->mapIndex);
										}
								}
							}
						}
						else
							Send01 ("Quest already loaded.", client);
					}
					else
						Send01 ("Only the leader of a team can start quests.", client);
				}
			}
			break;
		case 0xEF:
			if ( client->lobbyNum < 0x10 )
			{
				// Blocks

				unsigned blockNum;

				blockNum = 0x100 - selected;

				if (blockNum <= serverBlocks)
				{
					if ( blocks[blockNum - 1]->count < 180 )
					{
						if ((client->lobbyNum) && (client->lobbyNum < 0x10))
						{
							for (ch=0;ch<MAX_SAVED_LOBBIES;ch++)
							{
								if (savedlobbies[ch].guildcard == 0)
								{
									savedlobbies[ch].guildcard = client->guildcard;
									savedlobbies[ch].lobby = client->lobbyNum;
									break;
								}
							}
						}

						if (client->gotchardata)
						{
							client->character.playTime += (unsigned) servertime - client->connected; 
							ShipSend04 (0x02, client, logon);
							client->gotchardata = 0;
							client->released = 1;
							*(unsigned *) &client->releaseIP[0] = *(unsigned *) &serverIP[0];
							client->releasePort = serverPort + blockNum;
						}
						else
							Send19 (serverIP[0], serverIP[1], serverIP[2], serverIP[3],
							serverPort + blockNum, client);
					}
					else
					{
						Send01 ("Block is full.", client);
						Send07 (client);
					}
				}
			}
			break;
		case 0xFF:
			if ( client->lobbyNum < 0x10 )
			{
				// Ship select
				if ( selected == 0x00 )
					ShipSend0D (0x00, client, logon);
				else
					// Ships
					for (ch=0;ch<totalShips;ch++)
					{
						if (full_select == shipdata[ch].shipID)
						{
							if (client->gotchardata)
							{
								client->character.playTime += (unsigned) servertime - client->connected;
								ShipSend04 (0x02, client, logon);
								client->gotchardata = 0;
								client->released = 1;
								*(unsigned *) &client->releaseIP[0] = *(unsigned*) &shipdata[ch].ipaddr[0];
								client->releasePort = shipdata[ch].port;
							}
							else
								Send19 (shipdata[ch].ipaddr[0], shipdata[ch].ipaddr[1], 
								shipdata[ch].ipaddr[2], shipdata[ch].ipaddr[3],
								shipdata[ch].port, client);

							break;
						}
					}
			}
			break;
		default:
			break;
		}
	}
}

void CommandD9 (PSO_CLIENT* client)
{
	unsigned short *n;
	unsigned short *g;
	unsigned short s = 2;


	// Client writing to info board

	n = (unsigned short*) &client->decryptbuf[0x0A];
	g = (unsigned short*) &client->character.GCBoard[0];

	*(g++) = 0x0009;

	while ((*n != 0x0000) && (s < 85))
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*(g++) = 0x0020;
		else
			*(g++) = *n;
		n++;
		s++;
	}
	// null terminate
	*(g++) = 0x0000;
}

void CommandE8 (PSO_CLIENT* client)
{
	unsigned gcn;


	switch (client->decryptbuf[0x03])
	{
	case 0x04:
		{
			// Accepting sent guild card
			LOBBY* l;
			PSO_CLIENT* lClient;
			unsigned ch, maxch;

			if (!client->lobby)
				break;

			l = (LOBBY*) client->lobby;
			gcn = *(unsigned*) &client->decryptbuf[0x08];
			if ( client->lobbyNum < 0x10 )
				maxch = 12;
			else
				maxch = 4;
			for (ch=0;ch<maxch;ch++)
			{
				if ((l->clients[ch]) && (l->clients[ch]->character.guildCard == gcn))
				{
					lClient = l->clients[ch];
					if (prepped_guildcard(lClient->guildcard,client->guildcard))
					{
						AddGuildCard (client->guildcard, gcn, &client->decryptbuf[0x0C], &client->decryptbuf[0x5C],
							client->decryptbuf[0x10E], client->decryptbuf[0x10F], logon );
					}
					break;
				}
			}
		}
		break;
	case 0x05:
		// Deleting a guild card
		gcn = *(unsigned*) &client->decryptbuf[0x08];
		DeleteGuildCard (client->guildcard, gcn, logon);
		break;
	case 0x06:
		// Setting guild card text
		{
			unsigned short *n;
			unsigned short *g;
			unsigned short s = 2;

			// Client writing to info board

			n = (unsigned short*) &client->decryptbuf[0x5E];
			g = (unsigned short*) &client->character.guildcard_text[0];

			*(g++) = 0x0009;

			while ((*n != 0x0000) && (s < 85))
			{
				if ((*n == 0x0009) || (*n == 0x000A))
					*(g++) = 0x0020;
				else
					*(g++) = *n;
				n++;
				s++;
			}
			// null terminate
			*(g++) = 0x0000;
		}
		break;
	case 0x07:
		// Add blocked user
		// User @ 0x08, Name of User @ 0x0C
		break;
	case 0x08:
		// Remove blocked user
		// User @ 0x08
		break;
	case 0x09:
		// Write comment on user
		// E8 09 writing a comment on a user...  not sure were comment goes in the DC packet... 
		// User @ 0x08 comment @ 0x0C
		gcn = *(unsigned*) &client->decryptbuf[0x08];
		ModifyGuildCardComment (client->guildcard, gcn, (unsigned short*) &client->decryptbuf[0x0E], logon);
		break;
	case 0x0A:
		// Sort guild card
		// (Moves from one position to another)
		SortGuildCard (client, logon);
		break;
	}
}

void CommandD8 (PSO_CLIENT* client)
{
	unsigned ch,maxch;
	unsigned short D8Offset;
	unsigned char totalClients = 0;
	LOBBY* l;
	PSO_CLIENT* lClient;

	if (!client->lobby)
		return;

	memset (&PacketData[0], 0, 8);

	PacketData[0x02] = 0xD8;
	D8Offset = 8;

	l = client->lobby;

	if (client->lobbyNum < 0x10)
		maxch = 12;
	else
		maxch = 4;

	for (ch=0;ch<maxch;ch++)
	{
		if ((l->slot_use[ch]) && (l->clients[ch]))
		{
			totalClients++;
			lClient = l->clients[ch];
			memcpy (&PacketData[D8Offset], &lClient->character.name[4], 20 );
			D8Offset += 0x20;
			memcpy (&PacketData[D8Offset], &lClient->character.GCBoard[0], 172 );
			D8Offset += 0x158;
		}
	}
	PacketData[0x04] = totalClients;
	*(unsigned short*) &PacketData[0x00] = (unsigned short) D8Offset;
	cipher_ptr = &client->server_cipher;
	encryptcopy (client, &PacketData[0], D8Offset);
}

void Command81 (PSO_CLIENT* client, PSO_SERVER* ship)
{
	unsigned short* n;

	ship->encryptbuf[0x00] = 0x08;
	ship->encryptbuf[0x01] = 0x03;
	memcpy (&ship->encryptbuf[0x02], &client->decryptbuf[0x00], 0x45C);
	*(unsigned*) &ship->encryptbuf[0x0E] = client->guildcard;
	memcpy (&ship->encryptbuf[0x12], &client->character.name[0], 24);
	n = (unsigned short*) &ship->encryptbuf[0x62];
	while (*n != 0x0000)
	{
		if ((*n == 0x0009) || (*n == 0x000A))
			*n = 0x0020;
		n++;
	}
	*n = 0x0000;
	*(unsigned*) &ship->encryptbuf[0x45E] = client->character.teamID;
	compressShipPacket ( ship, &ship->encryptbuf[0x00], 0x462 );
}

void CommandEA (PSO_CLIENT* client, PSO_SERVER* ship)
{
	unsigned connectNum;

	if ((client->decryptbuf[0x03] < 32) && ((unsigned) servertime - client->team_cooldown[client->decryptbuf[0x03]] >= 1))
	{
		client->team_cooldown[client->decryptbuf[0x03]] = (unsigned) servertime;
		switch (client->decryptbuf[0x03])
		{
		case 0x01:
			// Create team
			if (client->character.teamID == 0)
				CreateTeam ((unsigned short*) &client->decryptbuf[0x0C], client->guildcard, ship);
			break;
		case 0x03:
			// Add a team member
			{
				PSO_CLIENT* tClient;
				unsigned gcn, ch;

				if ((client->character.teamID != 0) && (client->character.privilegeLevel >= 0x30))
				{
					gcn = *(unsigned*) &client->decryptbuf[0x08];
					for (ch=0;ch<serverNumConnections;ch++)
					{
						connectNum = serverConnectionList[ch];
						if (connections[connectNum]->guildcard == gcn)
						{
							if ( ( connections[connectNum]->character.teamID == 0 ) && ( connections[connectNum]->teamaccept == 1 ) )
							{
								AddTeamMember ( client->character.teamID, gcn, ship );
								tClient = connections[connectNum];
								tClient->teamaccept = 0;
								memset ( &tClient->character.guildCard2, 0, 2108 );
								tClient->character.teamID = client->character.teamID;
								tClient->character.privilegeLevel = 0;
								tClient->character.unknown15 = client->character.unknown15;
								memcpy ( &tClient->character.teamName[0], &client->character.teamName[0], 28 );
								memcpy ( &tClient->character.teamFlag[0], &client->character.teamFlag[0], 2048 );
								*(long long*) &tClient->character.teamRewards[0] = *(long long*) &client->character.teamRewards[0];
								if ( tClient->lobbyNum < 0x10 )
									SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
								else
									SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
								SendEA ( 0x12, tClient );
								SendEA ( 0x04, client );
								SendEA ( 0x04, tClient );
								break;
							}
							else
								Send01 ("Player already\nbelongs to a team!", client);
						}
					}
				}
			}
			break;
		case 0x05:
			// Remove member from team
			if (client->character.teamID != 0)
			{
				unsigned gcn,ch;
				PSO_CLIENT* tClient;

				gcn = *(unsigned*) &client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						RemoveTeamMember (client->character.teamID, gcn, ship);
						SendEA ( 0x06, client );
						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if ( tClient->character.privilegeLevel < client->character.privilegeLevel )
								{
									memset (&tClient->character.guildCard2, 0, 2108);
									memset (&client->encryptbuf[0x00], 0, 0x40);
									client->encryptbuf[0x00] = 0x40;
									client->encryptbuf[0x02] = 0xEA;
									client->encryptbuf[0x03] = 0x12;
									*(unsigned *) &client->encryptbuf[0x0C] = tClient->guildcard;
									if ( tClient->lobbyNum < 0x10 )
									{
										SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
										SendToLobby ( tClient->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
									}
									else
									{
										SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
										SendToLobby ( tClient->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
									}
									Send01 ("Member removed.", client);
								}
								else
									Send01 ("Your privilege level is\ntoo low.", client);
								break;
							}
						}
					}
					else
						Send01 ("Your privilege level is\ntoo low.", client);
				}
				else
				{
					RemoveTeamMember ( client->character.teamID, gcn, ship );
					memset (&client->character.guildCard2, 0, 2108);
					memset (&client->encryptbuf[0x00], 0, 0x40);
					client->encryptbuf[0x00] = 0x40;
					client->encryptbuf[0x02] = 0xEA;
					client->encryptbuf[0x03] = 0x12;
					*(unsigned *) &client->encryptbuf[0x0C] = client->guildcard;
					if ( client->lobbyNum < 0x10 )
					{
						SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						SendToLobby ( client->lobby, 12, &client->encryptbuf[0x00], 0x40, 0 );
					}
					else
					{
						SendToLobby ( client->lobby, 4, MakePacketEA15 ( client ), 2152, 0 );
						SendToLobby ( client->lobby, 4, &client->encryptbuf[0x00], 0x40, 0 );
					}
				}
			}
			break;
		case 0x07:
			if (client->character.teamID != 0)
			{
				unsigned short size;
				unsigned short *n;

				size = *(unsigned short*) &client->decryptbuf[0x00];

				if (size > 0x2B)
				{
					n = (unsigned short*) &client->decryptbuf[0x2C];
					while (*n != 0x0000)
					{
						if ((*n == 0x0009) || (*n == 0x000A))
							*n = 0x0020;
						n++;
					}
					TeamChat ((unsigned short*) &client->decryptbuf[0x00], size, client->character.teamID, ship);
				}
			}
			break;
		case 0x08:
			// Member Promotion / Demotion / Expulsion / Master Transfer
			//
			if (client->character.teamID != 0)
				RequestTeamList (client->character.teamID, client->guildcard, ship);
			break;
		case 0x0D:
			SendEA (0x0E, client);
			break;
		case 0x0F:
			// Set flag
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
				UpdateTeamFlag (&client->decryptbuf[0x08], client->character.teamID, ship);
			break;
		case 0x10:
			// Dissolve team
			if ((client->character.privilegeLevel == 0x40) && (client->character.teamID != 0))
			{
				DissolveTeam (client->character.teamID, ship);
				SendEA ( 0x10, client );
				memset ( &client->character.guildCard2, 0, 2108 );
				SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
				SendEA ( 0x12, client );
			}
			break;
		case 0x11:
			// Promote member
			if (client->character.teamID != 0)
			{
				unsigned gcn, ch;
				PSO_CLIENT* tClient;

				gcn = *(unsigned*) &client->decryptbuf[0x08];

				if (gcn != client->guildcard)
				{
					if (client->character.privilegeLevel == 0x40)
					{
						PromoteTeamMember (client->character.teamID, gcn, client->decryptbuf[0x04], ship);

						if (client->decryptbuf[0x04] == 0x40)
						{
							// Master Transfer
							PromoteTeamMember (client->character.teamID, client->guildcard, 0x30, ship);
							client->character.privilegeLevel = 0x30;
							SendToLobby ( client->lobby, 12, MakePacketEA15 ( client ), 2152, 0 );
						}

						for (ch=0;ch<serverNumConnections;ch++)
						{
							connectNum = serverConnectionList[ch];
							if (connections[connectNum]->guildcard == gcn)
							{
								tClient = connections[connectNum];
								if (tClient->character.privilegeLevel != client->decryptbuf[0x04]) // only if changed
								{
									tClient->character.privilegeLevel = client->decryptbuf[0x04];
									if ( tClient->lobbyNum < 0x10 )
										SendToLobby ( tClient->lobby, 12, MakePacketEA15 ( tClient ), 2152, 0 );
									else
										SendToLobby ( tClient->lobby, 4, MakePacketEA15 ( tClient ), 2152, 0 );
								}
								SendEA ( 0x12, tClient );
								SendEA ( 0x11, client );
								break;
							}
						}
					}
				}
			}
			break;
		case 0x13:
			// A type of lobby list...
			SendEA (0x13, client);
			break;
		case 0x14:
			// Do nothing.
			break;
		case 0x18:
			// Buying privileges and point information
			SendEA (0x18, client);
			break;
		case 0x19:
			// Privilege list
			SendEA (0x19, client);
			break;
		case 0x1C:
			// Ranking
			Send1A ("Tethealla Ship Server coded by Sodaboy\nhttp://www.pioneer2.net/\n\nEnjoy!", client );
			break;
		case 0x1A:
			SendEA (0x1A, client);
			break;
		default:
			break;
		}
	}
}
