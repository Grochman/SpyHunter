#define _USE_MATH_DEFINES
#include<math.h>
#include<stdio.h>
#include<string.h>
#include <time.h>
#include <stdlib.h>

extern "C" {
#include"./SDL2-2.0.10/include/SDL.h"
#include"./SDL2-2.0.10/include/SDL_main.h"
}

#ifdef __cplusplus
extern "C"
#endif

#define SCREEN_WIDTH	640
#define SCREEN_HEIGHT	480

#define CZARNY SDL_MapRGB(screen->format, 0x00, 0x00, 0x00)
#define ZIELONY SDL_MapRGB(screen->format, 0x00, 0xFF, 0x00)
#define CZERWONY SDL_MapRGB(screen->format, 0xFF, 0x00, 0x00)
#define NIEBIESKI SDL_MapRGB(screen->format, 0x11, 0x11, 0xCC)

#define WIELKOSC_PLANSZY 100
#define CZAS_OCHRONY 0
#define PKT_FOR_HEALTH 10
#define ILOSC_PRZECIWNIKOW 5
#define COOLDOWN_TIME 10
#define SAFE_TIME 1

enum typsamochodu {
	NPC,
	WROG
};

struct hitbox {
	double x;
	double y;
	double l;
	double w;
};
struct bulet {
	struct hitbox wymiary;
	int speed_y;
	int amocount;
	int range;
	int exist;
};
struct box {
	struct hitbox wymiary;
};
struct sides {
	SDL_Surface* sprite;
	double x1;
	double x2;
	double y;
};
struct car {
	SDL_Surface* sprite;
	struct hitbox wymiary;
	double speed_x;
	double constspeed_x;
	double speed_y;
	double constspeed_y;
	int lives;
	double pktforheart;
	typsamochodu cartype;
	struct bulet strzal;
};
struct game {
	int t1, t2, quit, rc, newgame, pause, gameover, crashFlag, shoot;
	double delta, worldTime, pkt, penaltyCooldown, safeCooldown;
};
struct powerup {
	SDL_Surface* sprite;
	struct hitbox wymiary;
	int exist;
};

// narysowanie napisu txt na powierzchni screen, zaczynajπc od punktu (x, y)
// charset to bitmapa 128x128 zawierajπca znaki
void DrawString(SDL_Surface *screen, int x, int y, const char *text, SDL_Surface *charset) {
	int px, py, c;
	SDL_Rect s, d;
	s.w = 8;
	s.h = 8;
	d.w = 8;
	d.h = 8;
	while(*text) {
		c = *text & 255;
		px = (c % 16) * 8;
		py = (c / 16) * 8;
		s.x = px;
		s.y = py;
		d.x = x;
		d.y = y;
		SDL_BlitSurface(charset, &s, screen, &d);
		x += 8;
		text++;
		};
	};

// narysowanie na ekranie screen powierzchni sprite w punkcie (x, y)
// (x, y) to punkt úrodka obrazka sprite na ekranie
void DrawSurface(SDL_Surface *screen, SDL_Surface *sprite, int x, int y) {
	SDL_Rect dest;
	dest.x = x - sprite->w / 2;
	dest.y = y - sprite->h / 2;
	dest.w = sprite->w;
	dest.h = sprite->h;
	SDL_BlitSurface(sprite, NULL, screen, &dest);
	};

// rysowanie pojedynczego pixela
void DrawPixel(SDL_Surface *surface, int x, int y, Uint32 color) {
	if (x < SCREEN_WIDTH && x>0 && y > 0 && y < SCREEN_HEIGHT)
	{
		int bpp = surface->format->BytesPerPixel;
		Uint8* p = (Uint8*)surface->pixels + y * surface->pitch + x * bpp;
		*(Uint32*)p = color;
	}
	};

// rysowanie linii o d≥ugoúci l w pionie (gdy dx = 0, dy = 1) bπdü poziomie (gdy dx = 1, dy = 0)
void DrawLine(SDL_Surface *screen, int x, int y, int l, int dx, int dy, Uint32 color) {
	for(int i = 0; i < l; i++) {
		
		DrawPixel(screen, x, y, color);
		x += dx;
		y += dy;
	};
};

// rysowanie prostokπta o d≥ugoúci bokÛw l i k
void DrawRectangle(SDL_Surface *screen, int x, int y, int l, int k, Uint32 outlineColor, Uint32 fillColor) {
	int i;
	DrawLine(screen, x, y, k, 0, 1, outlineColor);
	DrawLine(screen, x + l - 1, y, k, 0, 1, outlineColor);
	DrawLine(screen, x, y, l, 1, 0, outlineColor);
	DrawLine(screen, x, y + k - 1, l, 1, 0, outlineColor);
	for(i = y + 1; i < y + k - 1; i++)
		DrawLine(screen, x + 1, i, l - 2, 1, 0, fillColor);
	};

//zwolnienie pamieci programu
void FreeProgramSpace(SDL_Texture* scrtex, SDL_Renderer* renderer, SDL_Window* window) {
	SDL_DestroyTexture(scrtex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);
	SDL_Quit();
}

void Ui(SDL_Surface* screen, double pkt, double worldTime, SDL_Surface* charset, struct car p1) {
	char text[128];
	DrawRectangle(screen, 4, 4, SCREEN_WIDTH - 8, 36, CZERWONY, NIEBIESKI);

	if (worldTime < CZAS_OCHRONY) {
		sprintf(text, "Stanislaw Grochowski 193246 | czas trwania = %.1lf s pkt = %.lf00", worldTime, pkt);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
	}
	else {
		sprintf(text, "Stanislaw Grochowski 193246 | czas trwania = %.1lf s pkt = %.lf00 <3 = %i", worldTime, pkt, p1.lives);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 10, text, charset);
	}

	sprintf(text, "esc - wyjscie, n - nowa gra, p - pauza, f - koniec gry");
	DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, 26, text, charset);

	sprintf(text, "zaimplementowane funkcje: a, b, c, d, e, f, (h), i, j, k, l, m");
	DrawString(screen, screen->w - strlen(text) * 8 - 10, screen->h - 15, text, charset);
}

int MapColision(struct box plansza[], struct car* player) {
	int index = -1;
	for (int i = 0; i < WIELKOSC_PLANSZY; i++) {
		if (plansza[i].wymiary.y + plansza[i].wymiary.l >= player->wymiary.y - player->wymiary.l / 2 && 
			plansza[i].wymiary.y + plansza[i].wymiary.l <= player->wymiary.y + player->wymiary.l / 2)
			index = i;
	}
	if (index > -1) {
		if (plansza[index].wymiary.w >= player->wymiary.x - player->wymiary.w / 2 || 
			SCREEN_WIDTH - plansza[index].wymiary.w <= player->wymiary.x + player->wymiary.w / 2)
			return 1;
	}
	return 0;
}
int Crash(int x, int w) {
	if (x - w / 2 <= 10 || x + w / 2 >= SCREEN_WIDTH - 10) return 1;
	else return 0;
}
int CarInRange(struct car p1, struct car enemy) {
	int distance_y = NULL;

	if (enemy.wymiary.y > p1.wymiary.y)
		distance_y = enemy.wymiary.y - enemy.wymiary.l / 2 - p1.wymiary.y - p1.wymiary.l / 2;
	else
		distance_y = p1.wymiary.y - enemy.wymiary.y - enemy.wymiary.l / 2 - p1.wymiary.l / 2;
	
	return distance_y;
}
int Colision(struct hitbox wymiary1, struct hitbox wymiary2) {
	int distance_y;
	int distance_x;
	distance_y = wymiary1.y - wymiary2.y;
	if (abs(distance_y) < wymiary1.l / 2 + wymiary2.l / 2) {
		distance_x = wymiary1.x - wymiary2.x;
		if (abs(distance_x) < wymiary1.w / 2 + wymiary2.w / 2)
			return 1;
	}
	return 0;
}

void MoveEnemy(struct car enemy[], int ktory) {
	int goodposition = 0;
	enemy[ktory].wymiary.y = (rand() % (SCREEN_HEIGHT * 2)) + SCREEN_HEIGHT;
	while (goodposition == 0) {
		for (int i = 0; i < ILOSC_PRZECIWNIKOW; i++) {
			if (CarInRange(enemy[ktory], enemy[i]) <= 0 && ktory != i) {
				enemy[ktory].wymiary.y = (rand() % (SCREEN_HEIGHT * 2)) + SCREEN_HEIGHT;
				goodposition = 0;
				break;
			}
			else {
				goodposition = 1;
			}
		}
	}
}
void EnemySetup(struct car enemy[]) {
	
	for (int i = 0; i < ILOSC_PRZECIWNIKOW; i++) {	
		enemy[i].wymiary.l = 50;
		enemy[i].wymiary.w = 40;
		enemy[i].wymiary.x = SCREEN_WIDTH - 100 - (rand() % 540);
		enemy[i].wymiary.y = 30 - i * 200;
		enemy[i].constspeed_x = 300;
		enemy[i].speed_x = enemy[i].constspeed_x;
		enemy[i].constspeed_y = 450;
		enemy[i].speed_y = enemy[i].constspeed_y;
		if (i % 2 == 0) {
			enemy[i].cartype = WROG;
			enemy[i].sprite = SDL_LoadBMP("./car1.bmp");
		}
		else {
			enemy[i].cartype = NPC;
			enemy[i].sprite = SDL_LoadBMP("./car3.bmp");
		}
	}
}
int UpdateEnemy(struct car enemy[], double delta, struct car p1, struct box plansza[]) {
	int crashFlag = 0;
	for (int i = 0; i < ILOSC_PRZECIWNIKOW; i++) {
		enemy[i].wymiary.y += -enemy[i].speed_y * delta + p1.speed_y;
		
		if (enemy[i].wymiary.y > SCREEN_HEIGHT * 3) {
			enemy[i].wymiary.y = -SCREEN_HEIGHT;
			enemy[i].wymiary.x = SCREEN_WIDTH - 100 - (rand() % 540);
		}	
		else if (enemy[i].wymiary.y < -SCREEN_HEIGHT * 2) {
			enemy[i].wymiary.y = SCREEN_HEIGHT * 2;
			enemy[i].wymiary.x = SCREEN_WIDTH - 100 - (rand() % 540);
		}
		
		if (MapColision(plansza, &enemy[i]) == 1) {
			if (enemy[i].wymiary.x < SCREEN_WIDTH / 2) enemy[i].wymiary.x += enemy[i].speed_x * delta;
			else enemy[i].wymiary.x -= enemy[i].speed_x * delta;
		}
		else if (CarInRange(p1, enemy[i]) < 0 && enemy[i].cartype == WROG) {
			
			if (p1.wymiary.x < enemy[i].wymiary.x) enemy[i].wymiary.x -= enemy[i].speed_x * delta;
			else if (p1.wymiary.x > enemy[i].wymiary.x) enemy[i].wymiary.x += enemy[i].speed_x * delta;
			
			if (MapColision(plansza, &enemy[i]) == 1) {
				if (enemy[i].wymiary.x < SCREEN_WIDTH / 2) enemy[i].wymiary.x += enemy[i].speed_x * delta;
				else enemy[i].wymiary.x -= enemy[i].speed_x * delta;
			}
			
			if (Colision(enemy[i].wymiary, p1.wymiary) == 1) crashFlag = 1;
		}
		else if (enemy[i].cartype == NPC && CarInRange(p1, enemy[i]) < 0 && Colision(enemy[i].wymiary, p1.wymiary) == 1) {
			MoveEnemy(enemy, i);
			crashFlag = 2;
		}
	}
	return crashFlag;
}
void DrawEnemys(SDL_Surface* screen, struct car enemy[]) {
	for (int i = 0; i < ILOSC_PRZECIWNIKOW; i++) {
		DrawSurface(screen, enemy[i].sprite, enemy[i].wymiary.x, enemy[i].wymiary.y);
	}
}
void CrashUpdate(struct game* mygame, struct car enemy[], struct car* p1, struct box plansza[]) {
	if (Crash(p1->wymiary.x, p1->wymiary.w) == 1 || mygame->crashFlag == 1) {
		p1->speed_y = 0;
		p1->speed_x = 0;
		p1->wymiary.x = SCREEN_WIDTH - 70;
		if (mygame->worldTime > CZAS_OCHRONY && mygame->safeCooldown <= 0) {
			p1->lives--;
			if (p1->lives < 1) mygame->gameover = 1;
			mygame->safeCooldown = SAFE_TIME;
		}
	}
	else if (mygame->crashFlag == 2) mygame->penaltyCooldown = COOLDOWN_TIME;
	else if (p1->speed_y != 0 && MapColision(plansza, p1) != 1 && mygame->penaltyCooldown <= 0) {
		mygame->pkt += mygame->delta;
		if (mygame->worldTime > CZAS_OCHRONY) {
			p1->pktforheart += mygame->delta;
			if (p1->pktforheart > PKT_FOR_HEALTH) {
				p1->pktforheart = 0;
				p1->lives++;
			}
		}
	}
}

void StartShoot(struct car* p1) {
	p1->strzal.exist = 1;
	p1->strzal.wymiary.x = p1->wymiary.x - 3;
	p1->strzal.wymiary.y = p1->wymiary.y - p1->wymiary.l / 2;
	p1->strzal.wymiary.w = 10;
	p1->strzal.wymiary.l = 10;
}
int Shoot(struct car* p1, double delta, SDL_Surface* screen, struct car enemy[]) {
	if (p1->strzal.exist == 1) {
		if (p1->strzal.wymiary.y > p1->strzal.range) {
			p1->strzal.wymiary.y -= delta * p1->strzal.speed_y;
			DrawRectangle(screen, p1->strzal.wymiary.x, p1->strzal.wymiary.y, 6, 6, ZIELONY, CZERWONY);

			for (int i = 0; i < ILOSC_PRZECIWNIKOW; i++) {
				if(Colision(enemy[i].wymiary, p1->strzal.wymiary)) {
					p1->strzal.exist = 0;
					MoveEnemy(enemy, i);
					if (enemy[i].cartype == WROG)
						return 1;
					else
						return -1;
				}
			}
		}
		else {
			p1->strzal.exist = 0;
			if (p1->strzal.amocount > 0) p1->strzal.amocount--;
			if (p1->strzal.amocount == 0) p1->strzal.range = 100;
		}
	}
	return 0;
}
void ShootUpdate(struct game *mygame, struct car* p1, SDL_Surface* screen, struct car enemy[]) {
	mygame->shoot = Shoot(p1, mygame->delta, screen, enemy);
	switch (mygame->shoot) {
	case 1:
		if(mygame->penaltyCooldown<=0)
			mygame->pkt += mygame->shoot * 10;
		break;
	case -1:
		mygame->penaltyCooldown = COOLDOWN_TIME;
		break;
	}
}

void PowerUpSetup(struct powerup* power1) {
	power1->sprite = SDL_LoadBMP("./powerup.bmp");
	power1->wymiary.l = 30;
	power1->wymiary.w = 30;
	power1->wymiary.x = SCREEN_WIDTH / 2;
	power1->wymiary.y = -power1->wymiary.l;
	power1->exist = 0;
}
void UpdatePowerUp(struct powerup* power1, struct car* p1) {
	power1->wymiary.y += p1->speed_y;
	if (power1->wymiary.y > SCREEN_HEIGHT*5) power1->wymiary.y = -SCREEN_HEIGHT;
	if (Colision(power1->wymiary, p1->wymiary)) {
		p1->strzal.range = 0;
		p1->strzal.amocount = 3;
		power1->wymiary.y = -SCREEN_HEIGHT*6;
	}
}

void PlanszaSetup(struct box plansza[]) {
	for (int i = 0; i < WIELKOSC_PLANSZY; i++) {
		plansza[i].wymiary.x = 0;
		plansza[i].wymiary.y = i * (50) - WIELKOSC_PLANSZY * (50) + SCREEN_HEIGHT;
		plansza[i].wymiary.l = 100;
		if (i < 10)
			plansza[i].wymiary.w = 120;
		else if (i < 30)
			plansza[i].wymiary.w = 140;
		else if (i < 60)
			plansza[i].wymiary.w = 150;
		else if (i < 90)
			plansza[i].wymiary.w = 130;
		else
			plansza[i].wymiary.w = 120;
	}
}
void UpdatePlansza(struct box plansza[], double delta, struct sides* kamienie) {
	for (int i = 0; i < WIELKOSC_PLANSZY; i++) {
		plansza[i].wymiary.y += delta;
	}
	kamienie->x1 += delta;
	kamienie->x2 += delta;
	if (kamienie->x1 >= SCREEN_HEIGHT * 3 / 2) kamienie->x1 = kamienie->x2 - SCREEN_HEIGHT;
	if (kamienie->x2 >= SCREEN_HEIGHT * 3 / 2) kamienie->x2 = kamienie->x1 - SCREEN_HEIGHT;

	if (plansza[0].wymiary.y >= -10) PlanszaSetup(plansza);
}
void DrawPlansza(struct box plansza[], SDL_Surface* screen, struct sides kamienie) {
	for (int i = 0; i < WIELKOSC_PLANSZY; i++) {
		DrawRectangle(screen, plansza[i].wymiary.x, plansza[i].wymiary.y, plansza[i].wymiary.w, plansza[i].wymiary.l, ZIELONY, ZIELONY);
		DrawRectangle(screen,SCREEN_WIDTH - plansza[i].wymiary.x -plansza[i].wymiary.w, plansza[i].wymiary.y, plansza[i].wymiary.w, plansza[i].wymiary.l, ZIELONY, ZIELONY);
	}
	DrawSurface(screen, kamienie.sprite, SCREEN_WIDTH / 2, kamienie.x1);
	DrawSurface(screen, kamienie.sprite, SCREEN_WIDTH / 2, kamienie.x2);
}

void PlayerSetup(struct car* p1) {
	p1->sprite = SDL_LoadBMP("./car2.bmp");
	p1->wymiary.l = 50;
	p1->wymiary.w = 40;
	p1->wymiary.y = SCREEN_HEIGHT / 2 + 50;
	p1->wymiary.x = SCREEN_WIDTH / 2;
	p1->constspeed_x = 300;
	p1->constspeed_y = 700;
	p1->lives = 3;
	p1->pktforheart = 0;
	p1->speed_x = 0;
	p1->speed_y = 0;
	p1->strzal.speed_y = 300;
	p1->strzal.range = 100;
	p1->strzal.exist = 0;
	p1->strzal.amocount = 0;
}

void GameTimeUpdate(struct game* mygame) {
	mygame->worldTime += mygame->delta;
	if (mygame->penaltyCooldown > 0) mygame->penaltyCooldown -= mygame->delta;
	if (mygame->safeCooldown > 0) mygame->safeCooldown -= mygame->delta;
}
void GameSetup(struct game* mygame) {
	mygame->newgame = 0;
	mygame->pause = 0;
	mygame->gameover = 0;
	mygame->pkt = 0;
	mygame->worldTime = 0;
	mygame->crashFlag = 0;
	mygame->penaltyCooldown = 0;
	mygame->safeCooldown = 0;
	mygame->shoot = 0;
}
void GameOver(struct game *mygame, SDL_Event event, SDL_Renderer* renderer, SDL_Texture* scrtex, SDL_Surface* screen, struct car p1, SDL_Surface* charset) {
	if (mygame->gameover == 1) {
		char text[128];

		SDL_FillRect(screen, NULL, CZARNY);
		sprintf(text, "GAME OVER");
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 20, text, charset);
		sprintf(text, "time: %.1lf s points: %.lf00 lives: %i", mygame->worldTime, mygame->pkt, p1.lives);
		DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 10, text, charset);
		
		SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
		SDL_RenderCopy(renderer, scrtex, NULL, NULL);
		SDL_RenderPresent(renderer);
		
		mygame->newgame = 1;
		while (mygame->newgame == 1 && !mygame->quit) {
			while (SDL_PollEvent(&event)) {
				switch (event.type) {
				case SDL_KEYDOWN:
					if (event.key.keysym.sym == SDLK_ESCAPE) mygame->quit = 1;
					else if (event.key.keysym.sym == SDLK_n) mygame->newgame = 0;
					break;
				case SDL_QUIT:
					mygame->quit = 1;
					break;
				}
			}
		}
	}
}
void GetInput(SDL_Event event, struct game* mygame, struct car* p1) {
	switch (event.type) {
	case SDL_KEYDOWN:
		if (event.key.keysym.sym == SDLK_UP) p1->speed_y = mygame->delta * p1->constspeed_y;
		else if (event.key.keysym.sym == SDLK_RIGHT && p1->speed_y != 0) p1->speed_x = p1->constspeed_x;
		else if (event.key.keysym.sym == SDLK_LEFT && p1->speed_y != 0) p1->speed_x = -p1->constspeed_x;
		else if (event.key.keysym.sym == SDLK_SPACE && p1->strzal.exist == 0) StartShoot(p1);
		else if (event.key.keysym.sym == SDLK_p && mygame->pause == 1) mygame->pause = 0;
		else if (event.key.keysym.sym == SDLK_p && mygame->pause != 1) mygame->pause = 1;
		else if (event.key.keysym.sym == SDLK_n) mygame->newgame = 1;
		else if (event.key.keysym.sym == SDLK_f) mygame->gameover = 1;
		else if (event.key.keysym.sym == SDLK_ESCAPE) mygame->quit = 1;
		break;
	case SDL_KEYUP:
		if (event.key.keysym.sym == SDLK_UP) {
			p1->speed_y = 0;
			p1->speed_x = 0;
		}
		else if (event.key.keysym.sym == SDLK_LEFT && p1->speed_x < 0) p1->speed_x = 0;
		else if (event.key.keysym.sym == SDLK_RIGHT && p1->speed_x > 0) p1->speed_x = 0;
		break;
	case SDL_QUIT:
		mygame->quit = 1;
		break;
	};
}

int main(int argc, char **argv) {
	srand(time(NULL));
	char text[128];
	SDL_Event event;
	SDL_Surface *screen, *charset;
	SDL_Texture *scrtex;
	SDL_Window *window;
	SDL_Renderer *renderer;

	struct game mygame;
	struct box plansza[WIELKOSC_PLANSZY];
	struct car p1;
	struct car enemy[ILOSC_PRZECIWNIKOW];
	struct sides kamienie;
	struct powerup power1;

	//template-----------------
	if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
		printf("SDL_Init error: %s\n", SDL_GetError());
		return 1;
	}
	mygame.rc = SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
	if (mygame.rc != 0) {
		SDL_Quit();
		printf("SDL_CreateWindowAndRenderer error: %s\n", SDL_GetError());
		return 1;
	};
	SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
	SDL_RenderSetLogicalSize(renderer, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_SetWindowTitle(window, "Stanislaw Grochowski pop 2");
	screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);
	scrtex = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_WIDTH, SCREEN_HEIGHT);
	SDL_ShowCursor(SDL_DISABLE);
	charset = SDL_LoadBMP("./cs8x8.bmp");
	if(charset == NULL) {
		printf("SDL_LoadBMP(cs8x8.bmp) error: %s\n", SDL_GetError());
		SDL_FreeSurface(screen);
		FreeProgramSpace(scrtex, renderer, window);
		return 1;
	};
	SDL_SetColorKey(charset, true, 0x000000);
	
	kamienie.sprite = SDL_LoadBMP("./stones.bmp");
	mygame.quit = 0;
	
	while(!mygame.quit) {
		GameSetup(&mygame);
		PlanszaSetup(plansza);
		EnemySetup(enemy);
		PowerUpSetup(&power1);
		PlayerSetup(&p1);
		kamienie.x1 = SCREEN_HEIGHT / 2;
		kamienie.x2 = -SCREEN_HEIGHT / 2;
		mygame.t1 = SDL_GetTicks();
		
		while (!mygame.newgame && !mygame.quit && !mygame.gameover) {
			mygame.t2 = SDL_GetTicks();
			mygame.delta = (mygame.t2 - mygame.t1) * 0.001;
			mygame.t1 = mygame.t2;
			SDL_FillRect(screen, NULL, CZARNY);
			DrawPlansza(plansza, screen, kamienie);
			DrawSurface(screen, power1.sprite, power1.wymiary.x, power1.wymiary.y);
			DrawSurface(screen, p1.sprite, p1.wymiary.x, p1.wymiary.y);
			DrawEnemys(screen, enemy);
			Ui(screen, mygame.pkt, mygame.worldTime, charset, p1);

			if (mygame.pause != 1) {	
				GameTimeUpdate(&mygame);
				UpdatePlansza(plansza, p1.speed_y, &kamienie);
				UpdatePowerUp(&power1, &p1);

				p1.wymiary.x += p1.speed_x * mygame.delta;

				ShootUpdate(&mygame, &p1, screen, enemy);
				
				mygame.crashFlag = UpdateEnemy(enemy, mygame.delta, p1, plansza);
				CrashUpdate(&mygame, enemy, &p1, plansza);
			}
			else {
				sprintf(text, "PAUZA");
				DrawString(screen, screen->w / 2 - strlen(text) * 8 / 2, SCREEN_HEIGHT / 2 - 10, text, charset);
			}

			SDL_UpdateTexture(scrtex, NULL, screen->pixels, screen->pitch);
			SDL_RenderCopy(renderer, scrtex, NULL, NULL);
			SDL_RenderPresent(renderer);

			while (SDL_PollEvent(&event))
				GetInput(event, &mygame, &p1);
		};
		GameOver(&mygame, event, renderer, scrtex, screen, p1, charset);
	};
	SDL_FreeSurface(charset);
	SDL_FreeSurface(screen);
	FreeProgramSpace(scrtex, renderer, window);
	return 0;
	};
