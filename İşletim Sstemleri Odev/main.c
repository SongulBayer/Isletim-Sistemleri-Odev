#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_UZUNLUK 512

// Global variables
char prompt[32] = "Sau >";                                // varsayılan bilgi istemi
char PATH[32] = "/bin/bash";                                // varsayılan PATH istemi
char gecmisListesi[MAX_UZUNLUK][MAX_UZUNLUK];                   // Kullanıcının gireceği maksimum komut sayısıı
char gecmisKomut[MAX_UZUNLUK][20] = {{0}};                       // Eki komutların sayısı NUMBER_OF_WORDS][MAX_SIZE_OF_WORD]
char yeniKomut[MAX_UZUNLUK][20] = {{0}};                       // Yeniden adlandırılmış komutların sayısı [NUMBER_OF_WORDS][MAX_SIZE_OF_WORD]
int gecmisIterator = 1;                                    // Kullanıcının girdiği komutların sayısı
int komutIndex = 0;

//Fonksiyon prototipleri
bool komutCalismasi(char*, bool);
void boslukKirp(char*);
void gecmis(char*);
void ozelPrompt();
void komutDegisimi();
char* farkliIsimKomut(char*);

int main(int argc, char* argv[]) {
	bool kabukDurumu = true;                                // Kabuk kontrolu

	do {
		// ana kabuk döngüsü
		if (argc == 2) {
			
			FILE* batchDosya = fopen(argv[1], "r");
			if (batchDosya == NULL) {
				fprintf(stderr, "Hata: Batch dosyası açılamıyor.\n");
				return EXIT_FAILURE;
			}

			// batchfile'in boş olup olmadığını kontrol eder
			fseek(batchDosya, 0, SEEK_END);
			if (ftell(batchDosya) == 0) {
				fprintf(stderr, "Hata: Batch fdosyası boş.\n");
				return EXIT_FAILURE;
			}
			fseek(batchDosya, 0, SEEK_SET);

			// Batch mod başlığını ve ve etkin dosyayı yazdırır
			printf("-----[ BATCH MOD ]-----\n");
			printf("--> Dosya %s\n", argv[1]);

			char buf[MAX_UZUNLUK + 1];
			while (fgets(buf, MAX_UZUNLUK, batchDosya)) {
				if (buf[strlen(buf) - 1] != '\n') {
					fprintf(stderr, "Hata: En fazla 512 karakter olabilir\n");
					fflush(stdout);
					int ch;
					while ((ch = getc(batchDosya))) {
						if (ch == '\n') {
							break;
						}
					}
				}
				else {
					printf("\nBatch satir> %s", buf);
					komutCalismasi(buf, true);
				}
			}

			// Batch dosyası kapatılıyor
			fclose(batchDosya);
			kabukDurumu = false;
		}
		else if (argc > 2) {
			// Geçersiz program çağrısı
			fprintf(stderr, "Hata: Çok fazla arguman.\n");
			return EXIT_FAILURE;
		}
		else {
			// Kullanıcı girdisini işler.
			char kullaniciInput[MAX_UZUNLUK + 2];                 // Kullanıcıya göre dize girişini saklar
			static bool ilkGosterim = true;                 //Kabuk başlatıldığında hoş geldiniz istemi görüntüler

			if (ilkGosterim == true) {
				printf("***** INTERAKTİF MOD *****\n");
				printf("--> Tip \"help\" komutlari goruntulemek icin\n");
				printf("\n%s ", prompt);
				ilkGosterim = false;
			}
			else {
				printf("\n%s ", prompt);
			}

			// Kullanıcı girişi okur ve Ctrl-D 'yi işler.
			if (fgets(kullaniciInput, (MAX_UZUNLUK + 3), stdin) == NULL) {
				putchar('\n');
				fflush(stdout);
				return EXIT_SUCCESS;
			}
			else if (strlen(kullaniciInput) > 512 ) {
				fprintf(stderr, "Hata: %u: En fazla 512 karater olmalı\n", (unsigned)strlen(kullaniciInput));
				int ch;
				while ((ch = getc(stdin)) != '\n' || ch != EOF);
				continue;
			}

			// inputu ayrıştır ve çalıştırır
			kabukDurumu = komutCalismasi(kullaniciInput, false);
		}
	} while(kabukDurumu);

	return EXIT_SUCCESS;
}

bool komutCalismasi(char strInput[], bool batchMode) {
	gecmis(strInput);                                      // geçerli komutu geçmişe ekler
	int toplamCocuk = 0;                         
	// sayaç toplam alt komutları takip et
	
	char* komut = strtok(strInput, ";");                 
	pid_t pid;                                             
	bool cikisDurumu = true;                                 

	

	
	while (komut) {
		boslukKirp(komut);

		
		komut = farkliIsimKomut(komut);

		if (strcmp(komut, "") == 0) {
			
			komut = strtok(NULL, ";");
			continue;
		}
		else if (strncmp(komut, "cd ", 3) == 0 && batchMode == false) {
			
			if (chdir(komut + 3) != 0) {
				fprintf(stderr, "Hata: cd: gecersiz komut\n");
			}
			break;
		}
		if ( (strcmp(komut, "quit") == 0) || (strcmp(komut, "exit") == 0)) {
			// Exits parent shell
			cikisDurumu = false;
			komut = strtok(NULL, ";");
			continue;
		}
		else if ((strcmp(komut, "help") == 0)) {
			printf("/--------[ HELP: LIST OF INTERNAL komutS ]-------\\\n"
			       "| gecmis       - prints list of komuts entered  |\n"
			       "| prompt        - sets custom prompt string        |\n"
				   "| customize     - sets customized shell options    |\n"
				   "| path          - sets PATH directory              |\n"
				   "| cd            - change current directory         |\n"
				   "| quit OR exit  - exits shell program              |\n"
				   "\\--------------------------------------------------/\n");
			break;
		}
		else if (strcmp(komut, "path") == 0 && batchMode == false) {
	
			do {
				printf("Enter custom path: ");
				fgets(PATH, 32, stdin);
				if (PATH[strlen(PATH) - 1] != '\n') {
					fprintf(stderr, "Hata: Maksimutere izin verilir\n");
					int ch;
					while ((ch = getc(stdin)) != '\n' && ch != EOF);
				}
			} while (PATH[strlen(PATH) - 1] != '\n');
			PATH[strcspn (PATH, "\n")] = '\0';
			printf("PATH successfully updated\n");
			break;
		}
		else if (strcmp(komut, "prompt") == 0 && batchMode == false) {
			do {
				printf("Ozel bilgi istemini girin: ");
				fgets(prompt, 32, stdin);
				if (prompt[strlen(prompt) - 1] != '\n') {
					fprintf(stderr, "Error: Maximum 30 karakter \n");
					int ch;
					while ((ch = getc(stdin)) != '\n' && ch != EOF);
				}
			} while (prompt[strlen(prompt) - 1] != '\n');
			prompt[strcspn (prompt, "\n")] = '\0';  
			printf("Prompt başarıyla güncellendi\n");
			break;
		}
		else if(strstr(komut, "customize") != NULL) {
			ozelPrompt(); 
			break;
		}
		else if ((pid = fork()) == 0) {  
			// Çocuk proses
			execl(PATH, "Shell", "-c", komut, NULL);
			fprintf(stderr, "Error: %s: çalışma hatası\n", komut);
			_exit(EXIT_FAILURE);                   
		}
		else if (pid < 0) {
			// Çatallama başarısız oldu

			fprintf(stderr, "Error: Fork çocukta hata verdi\n");
			_exit(EXIT_FAILURE);   
		}
		else {  
			// evebeyn proses
			komut = strtok(NULL, ";");		
			toplamCocuk++;  
		}
	}

	for(int i = 0; i < toplamCocuk; ++i) {
		wait(NULL); 
	}

	return cikisDurumu; 
}

void boslukKirp(char* parsedInput) {
	char *s = parsedInput + strlen(parsedInput);
	while (--s >= parsedInput) {
		if (!isspace(*s)) {
			break;
		}
		*s = 0;
	}

	size_t n = 0;
	while (parsedInput[n] != '\0' && isspace((unsigned char)parsedInput[n])) {
		n++;
	}
	memmove(parsedInput, parsedInput + n, strlen(parsedInput) - n + 1);
}


void gecmis(char* cmdString) {
	
	strcpy(gecmisListesi[gecmisIterator], cmdString);
	gecmisIterator++;

	if(strncmp(cmdString, "gecmis", 7) == 0) {
		for(int i = 1; i < (gecmisIterator - 1); i++) {
			printf("   %i  %s", i, gecmisListesi[i]);
		}	
	}
}



void ozelPrompt(){
	printf("Ismi degistirmek istiyorsan 1'e basmalisin.");
	char kullaniciYaniti[2];                                                        
	scanf ("%[^\n]%*c", kullaniciYaniti);


	if(*kullaniciYaniti == '1'){
		printf("Komut arayuzu degisti...\n");
		sleep(2);
		komutDegisimi();                                                            // komut değişikliği istemine gider
	}
	else {
		printf("Gecersiz deger, komut istemine don...\n");                        // Geçersiz komut istemine dön
	}
}
void komutDegisimi(){
	bool durum = true;
	while(durum){
		komutIndex++;
		char inputDegis[100];
		
		printf("\e[1;1H\e[2J");                                                     // ekranı temizle
		printf("\e[91mEnter a komut, without flags, that you would like to change followed by its new name.\n");
		printf("Ex. 'ls [newName]'\n");
		
		printf("t ek> ");
		
		if(fgets(inputDegis,sizeof(inputDegis),stdin) != NULL){							// Kullanıcıdan gelen girdiyi okur.
			int kelimeSayisi = 0;
			for (char *p = strtok(inputDegis," "); p != NULL; p = strtok(NULL, " ")){	// komutları dizide saklayın.
				if(++kelimeSayisi == 1) 
					strcpy(gecmisKomut[komutIndex], p);										//eski komut 
				else if(kelimeSayisi == 2)
					strncpy(yeniKomut[komutIndex], p, strlen(p)-1);						// yeni komut
				else{
					printf("Cok fazla komut, komut eklenemiyor...\n");
					break;																// çok fazla komut
				}
			}
		}
		fflush(stdout);
		
		// Kullanıcı yanıtını saklar
		char kullaniciYaniti[2];
		printf("Baska bir komutu degistirmek isterseniz (1), cikmak isterseniz (0) basın?\ncustomize> ");
		scanf ("%[^\n]%*c", kullaniciYaniti);

		// Yanıt= 0 ise bilgi istemi arayüze dön
		if(*kullaniciYaniti == '0') durum = false;
		// response 1 ise başka bir komutu değiştirir
		else if(*kullaniciYaniti == '1') durum = true;
		else {			
			durum = false;
			sleep(2);
		}	
	}
	printf("Prompt donduruluyor...\n");
	sleep(2);	
	printf("\e[1;1H\e[2J"); //Ekranı temizle
}
char* farkliIsimKomut(char* komut){
	int i;
	// Girilen komutun farklı bir adı olup olmadığını kontrol eder, eğer öyleyse orijinal adıyla değiştirir
	for(i = 1; i <= komutIndex; i++){
		if (strcmp(yeniKomut[i], komut) == 0)
			return gecmisKomut[i];
	}
	return komut;
}
