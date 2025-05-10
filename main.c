#include <raylib.h> 
#include <math.h> 
#include <stdint.h> 
#include <stdio.h> 

#define RENDER_PLAYER_SOLID

#define WIDTH 1200 
#define HEIGHT 700 
#define SPAWN_LEFT   -200
#define SPAWN_RIGHT ( WIDTH + 200) 
#define SPAWN_TOP   -200
#define SPAWN_BOTTOM (HEIGHT + 200) 
#define RADIUS_P 15
#define MAX_VERTICES 6  
#define MAX_BULLETS 5
#define BULLET_SPEED 250 
#define ROTATION_SNAP 5 
#define ROTATIONS ( 360 / ROTATION_SNAP ) 
#define MAX_ASTEROIDS 20 
#define MIN_R 	  30 
#define MAX_R 	  40
#define MIN_SPEED 100 
#define MAX_SPEED 200 
#define MAX_PARTICLES 100
#define MAX_PART_LIFE 100 
#define THICKNESS  2.0
#define MAX_HEALTHS 3
#define IMMUNE_DURATION 60 //60 frames
			   
#define SCORE_TXT_MAX 5
#define FONTSIZE_S    32 
#define FONTSIZE_M    64 
#define FONTSIZE_L    72 

float cos_table [ ROTATIONS ] ;
float sin_table [ ROTATIONS ] ;
Vector2 vertices [ MAX_VERTICES * 2 ] ; 

enum SIZES{ SMALL, MEDIUM, LARGE } ;
enum GAMESTATES { MENU, MAIN, PAUSE, GAMEOVER }; 
enum PLAYER_STATES  { NONE, IMMUNE, DEAD } ; 
enum SOUNDS 	{ EMPTY, HIT,  SHOOT, EXPLODE, POWERUP };

typedef struct Bullet{ 
	bool    	active;
	Vector2 	vel;
	Vector2 	pos; //head 
} Bullet; 

struct Player{ 
	int		rotation;
	int		 eff_life; 	
	enum PLAYER_STATES  state ; 
	Vector2 	center;
	Vector2 	vertices[3] ; 
	Vector2 	velocity ;
}player; 

typedef struct Asteroid{ 
	Vector2 	center; 
	Vector2 	vertices[MAX_VERTICES]; 
	Vector2 	velocity; 
	int     	speed;
	bool    	active; 
	enum SIZES 	size  ; 
}Asteroid; 

struct GameState{ 
	char    score_text[ SCORE_TXT_MAX ] ;
	int 	healths ; 
	int 	score   ; 
	enum GAMESTATES state ; 	
	enum SOUNDS     current_sound ;

}Game; 

typedef struct Particle{ 
	Vector2 pos; 
	Vector2 vel; 
	int life   ; //count in frames 

}Particle; 


void initGame ( ); 
void renderGame( ) ; 
void updateGame( double dt ) ; 
void renderGameOver() ; 
void renderMenu ( ) ; 

void initTable( );
void initAestroid  ( Asteroid* ); 
void initDebris( Vector2 spawn_point );
void updateAsteroid( Asteroid*, double);
void renderAsteroid( Asteroid* ); 
void generateVertices ( Vector2 v[MAX_VERTICES], int , int, int );  

void initPlayer( int pos_x, int pos_y ); 
void renderPlayer( ); 
void updatePlayer( double );
bool checkCollisionPlayer( ) ; 
void updateBullet( Bullet* bullet, double delta ); 
void renderBullet( Bullet* bullet ); 

void renderUI ( ) ; 
void addParticles( int N, Vector2 pos); 
void updateParticle( Particle* p, double dt );  
void renderParticle( Particle* ) ; 
void updateScore ( int score ) ;  

Asteroid   asteroid_pool [ MAX_ASTEROIDS ] ; 
Bullet     bullets       [ MAX_BULLETS ];
Particle   particles      [ MAX_PARTICLES ] ;
Sound 	   sounds 	 [ 5 ] ; 

unsigned char ALPHA_GRADIENT  [ 100 ] ; 
const char* TEXT_GAME_OVER = "GAME OVER"; 
const char* TEXT_PROMPT_1 = "PRESS <ENTER> TO CONTINUE"; 
const char* TEXT_PROMPT_2 = "PRESS <ENTER>"; 
const char* TEXT_PAUSED   = "GAME PAUSED";
//i might add this later
unsigned int TEXT_SIZE_GAMEOVER ;
unsigned int TEXT_SIZE_PROMPT1  ; // press enter
unsigned int TEXT_SIZE_PAUSED  ; 
unsigned int TEXT_SIZE_PROMPT2; 

int main( void ) { 
	InitWindow( WIDTH, HEIGHT, "Asteroid"); 
	InitAudioDevice() ; 
	SetTargetFPS( 60 ); 

	TEXT_SIZE_GAMEOVER = MeasureText(  TEXT_GAME_OVER,   FONTSIZE_L ) ;
	TEXT_SIZE_PROMPT1  = MeasureText(  TEXT_PROMPT_1 ,   FONTSIZE_S ) ;
	TEXT_SIZE_PROMPT2  = MeasureText(  TEXT_PROMPT_2 ,   FONTSIZE_L ) ;
	TEXT_SIZE_PAUSED   = MeasureText(  TEXT_PAUSED,      FONTSIZE_M ) ;

	//loads the audio files
	sounds[ EXPLODE ] = LoadSound( "./res/explode.wav"); 
	sounds[ HIT ] = LoadSound( "./res/hit.wav"); 
	sounds[ SHOOT ] = LoadSound( "./res/shoot.wav"); 

	initTable(); 
	initGame() ; 
	Game.state = MENU ; 

	double delta ;
	while(!WindowShouldClose()){ 
		delta = GetFrameTime();
		
		switch( Game.state ){ 
			case MENU: 
				renderMenu () ; 
				if( IsKeyPressed( KEY_ENTER )) { 
					Game.state = MAIN; 
					break ; 
				}
				break ; 

			case MAIN: 
				if( IsKeyPressed( KEY_Q )){ 
					Game.state = PAUSE ; 
					break;
				}
				updateGame( delta ); 
				renderGame() ; 
				break; 
			case PAUSE: 	
				if( IsKeyPressed( KEY_ENTER ) ) 
					Game.state = MAIN; 		
				renderGame() ; 
				DrawText( TEXT_PAUSED, WIDTH / 2.0f - ( TEXT_SIZE_PAUSED / 2.0f), HEIGHT / 2 - FONTSIZE_M, FONTSIZE_M, WHITE);
				break; 

			case GAMEOVER: 
				if( IsKeyPressed( KEY_ENTER ) ) { 
					Game.state = MAIN; 
					initGame() ; 
				}
				renderGame() ; 
				renderGameOver() ; 
				break ; 
		}
	}
	return 0 ; 
}

void initGame( ) { 
	initPlayer( WIDTH / 2, HEIGHT / 2);
	for(int i = 0; i < 5; i++){ 
		Game.score_text[i] = '0'; 
	}
	int N = GetRandomValue( 10, MAX_ASTEROIDS - 10);
	for(int i = 0; i < N; i++){ 
		asteroid_pool[i].active = false; 
		initAestroid( &asteroid_pool[i]); 
	}

	Game.score_text[4] = '\0';
	Game.healths       = MAX_HEALTHS ; 
	Game.state 	   = MAIN; 
	Game.current_sound = EMPTY ; 
	return ;
}

void renderPlayer( ) { 
	if( player.state == IMMUNE )  
		if( GetRandomValue(0, 1) ) return ;
	DrawLineEx( player.vertices[0], player.vertices[1], THICKNESS, WHITE); 
	DrawLineEx( player.vertices[0], player.vertices[2], THICKNESS, WHITE);
	DrawLineEx( player.vertices[1], player.center, THICKNESS, WHITE); 
	DrawLineEx( player.vertices[2], player.center, THICKNESS, WHITE);	

	return;
}

void initPlayer( int pos_x, int pos_y){ 
	player.center = (Vector2){ pos_x, pos_y };
	player.velocity = (Vector2) { 0 }; 
	
	//tip 
	player.vertices[0].x = pos_x + cosf( 270 * DEG2RAD ) * RADIUS_P;
	player.vertices[0].y = pos_y + sinf( 270 * DEG2RAD)  * RADIUS_P;

	//base 1
	player.vertices[1].x = pos_x + cosf( 45 * DEG2RAD ) * RADIUS_P; 
	player.vertices[1].y = pos_y + sinf( 45 * DEG2RAD ) * RADIUS_P; 
	
	//base 2

	player.vertices[2].x = pos_x + cosf( 135 * DEG2RAD) * RADIUS_P; 
	player.vertices[2].y = pos_y + sinf( 135 * DEG2RAD) * RADIUS_P; 

	//initilize the bullets
	for(int i = 0; i < MAX_BULLETS; i++){ 
		bullets[i].active = false; 
	}
	return;
}


void updatePlayer( double dt ){ 
	if( player.state == IMMUNE ){ 
		player.eff_life -- ; 
		if( player.eff_life == 0)
			player.state = NONE; 
	}
	if( IsKeyPressed( KEY_SPACE )){ 
		Game.current_sound = SHOOT ; 
		for(int i = 0; i < MAX_BULLETS; i++){ 
			if( bullets[i].active) 
				continue; 
			bullets[i].active = true; 
			bullets[i].vel.x  = cosf( (270 + player.rotation) * DEG2RAD ) ; 
			bullets[i].vel.y  = sinf( (270 + player.rotation) * DEG2RAD ) ;
			bullets[i].pos.x  = player.vertices[0].x + 3; 
			bullets[i].pos.y  = player.vertices[0].y + 3; 
			break;
		}
	}	

	int index_dir = (270 + player.rotation) / ROTATION_SNAP ; 
	index_dir %= ROTATIONS; 
	if( IsKeyDown( KEY_UP )){ 
		player.velocity.x += cos_table[index_dir] * 5;
		player.velocity.y += sin_table[index_dir] * 5;
		if( player.velocity.x >= 100 ) 
			player.velocity.x = 100 ;
		if( player.velocity.y >= 100 ) 
			player.velocity.y = 100 ;
	}

	else if( IsKeyDown( KEY_DOWN)){ 
		player.velocity.x -= cos_table[index_dir] * 5;
		player.velocity.y -= sin_table[index_dir] * 5;
	}

	if ( IsKeyDown( KEY_RIGHT )){ 
		player.rotation += ROTATION_SNAP ;
	}
	else if ( IsKeyDown( KEY_LEFT )){ 
		player.rotation -= ROTATION_SNAP ;
	}

	player.rotation = fmodf( fmodf( player.rotation, 360) + 360, 360);
	player.center.x += player.velocity.x * dt;
	player.center.y += player.velocity.y * dt;
	if( player.center.x >= WIDTH){ 
		player.center.x = 0; 
	}
	else if( player.center.x <= 0){ 
		player.center.x = WIDTH; 
	}

	if( player.center.y >= HEIGHT){ 
		player.center.y = 0; 
	}
	else if( player.center.y <= 0){ 
		player.center.y = HEIGHT ; 
	}

	int index_tip = (270 + player.rotation ) / ROTATION_SNAP;
	index_tip %= ROTATIONS;
	player.vertices[0].x = player.center.x + cos_table[index_tip] * RADIUS_P;
	player.vertices[0].y = player.center.y + sin_table[index_tip] * RADIUS_P;

	int index_base = ( 45 + player.rotation ) / ROTATION_SNAP ; 
	index_base %= ROTATIONS;
	player.vertices[1].x = player.center.x + cos_table[ index_base ] * RADIUS_P; 
	player.vertices[1].y = player.center.y + sin_table[ index_base ] * RADIUS_P; 
	
	index_base = ( 135 + player.rotation ) / ROTATION_SNAP ; 
	index_base %= ROTATIONS;
	player.vertices[2].x = player.center.x + cos_table[ index_base ]  * RADIUS_P; 
	player.vertices[2].y = player.center.y + sin_table[ index_base ]  * RADIUS_P; 

	return;
}


void initTable(){ 
	for(int i = 0; i < ROTATIONS; i++){ 
		float rad = i * ROTATION_SNAP * DEG2RAD;
		cos_table[i] = cosf( rad ) ;
		sin_table[i] = sinf( rad ) ; 
	}

	return;
}

void initAestroid  ( Asteroid* a) { 
	if( a->active ) return ;
	int last_deg = 0;
	a->speed     = GetRandomValue( MIN_SPEED, MAX_SPEED ); 
	//asteroids must spawn offscreen 
	int random_side = GetRandomValue( 0, 3); 
	switch( random_side ){ 
		case 0:
			a->center = (Vector2){ GetRandomValue( SPAWN_LEFT, 0), 
				GetRandomValue( 0, HEIGHT) }; 
			break;
		case 1:
			a->center = (Vector2){ GetRandomValue( WIDTH, SPAWN_RIGHT), 
				GetRandomValue( 0, HEIGHT) }; 
			break;
		case 2:
			a->center = (Vector2){ GetRandomValue( 0, WIDTH), 
				GetRandomValue( SPAWN_TOP, 0) }; 
			break;
		case 3:
			a->center = (Vector2){ GetRandomValue( 0, WIDTH), 
				GetRandomValue( HEIGHT, SPAWN_BOTTOM) }; 
			break;

	} ;

	a->active = true; 
	a->size   = LARGE; 
	//target the player ( average ) 
	int target_x = GetRandomValue( player.center.x - 300, player.center.x + 300 );
	int target_y = GetRandomValue( player.center.y - 300, player.center.y + 300 );
	double dir   = atan2( target_y - a->center.y, target_x - a->center.x); 
	a->velocity.x = cosf( dir );
	a->velocity.y = sinf( dir );

	generateVertices ( a->vertices, a->center.x , a->center.y, MAX_R ) ; 
	return;
}

void initDebris( Vector2 spawn_point ) { 
	for(int y=0; y < MAX_ASTEROIDS; y++){ 		
		if( asteroid_pool[ y ].active ) continue ; 	
		Asteroid* a = &asteroid_pool[ y ] ;
		a->active = true;
		a->size   = SMALL;
		a->center = (Vector2) spawn_point; 
		int random_index = GetRandomValue(0, ROTATIONS - 1); 
		a->velocity.x = cos_table[ random_index ];
		a->velocity.y = sin_table[ random_index ];	
		generateVertices ( a->vertices, a->center.x , a->center.y , MAX_R - 20) ; 
		break ; 
	}
	return;
}

void generateVertices ( Vector2 v[MAX_VERTICES], int x, int y, int max_r){ 
	int deg = 0 ; 
	for(int i = 0; i < MAX_VERTICES; i++){ 
		int r = GetRandomValue( MIN_R, max_r); 
		deg += GetRandomValue(45, 60); 
		v[i] = (Vector2){ 
				x + cosf( deg * DEG2RAD) * r , 
				y + sinf( deg * DEG2RAD ) * r }; 
	}
	return ; 
}

void updateAsteroid ( Asteroid* a, double dt) { 
	//check collision with bullets 
	//when hit --> divides into smaller asteroids 
	if( !a->active ) return ;

	a->center.x += a->velocity.x * a->speed * dt;  
	a->center.y += a->velocity.y * a->speed * dt; 
	if( (a->center.x > SPAWN_RIGHT || a->center.x < SPAWN_LEFT) || \
			(a->center.y > SPAWN_BOTTOM || a->center.y < SPAWN_TOP)){ 
		a->active = false;
		initAestroid( a ); // reinitialize the asteroid 
		return;
	}

	for(int i = 0; i < MAX_VERTICES; i++){ 
		a->vertices[i].x += a->velocity.x * a->speed * dt;
		a->vertices[i].y += a->velocity.y * a->speed * dt;
	}


	return;
}

void renderAsteroid ( Asteroid* a){ 
	if(a->active != true) return;
	for(int i = 0; i < MAX_VERTICES ; i++){ 
		DrawLineEx( a->vertices[i], a->vertices[(i+1) % MAX_VERTICES], 2.00, WHITE); 
		
	}
	return; 
}

void updateBullet( Bullet* bullet, double delta ){ 
	if( bullet->active == false ) return; 

	bullet->pos.x += bullet->vel.x * delta * BULLET_SPEED ; 
	bullet->pos.y += bullet->vel.y * delta * BULLET_SPEED ; 

	if( bullet->pos.x >= WIDTH || bullet->pos.x < 0) { 
		bullet->active = false; 
	}
	else if( bullet->pos.y >= HEIGHT || bullet->pos.y < 0){ 
		bullet->active = false; 
	}
	return ;
}

void renderBullet( Bullet* bullet ){ 
	if(!bullet->active ) return ;
	DrawLineEx( bullet->pos, (Vector2){ 
			bullet->pos.x - bullet->vel.x * 15, 
			bullet->pos.y - bullet->vel.y * 15}, 
			2.0, WHITE); 			
}

void renderUI ( ) { 
	int x = WIDTH - 100; 
	int y = HEIGHT - 78; 
   	for(int i = 0; i < Game.healths; i++){ 	
		DrawPoly( (Vector2){x + (32 * i), y}, 3, 15, 270, WHITE); 
	}
	DrawText(   Game.score_text, WIDTH - 128, HEIGHT - 64, 48, WHITE);	
	return ; 
}


void updateScore ( int score ) { 
	Game.score += score ; 
	int temp = Game.score ; 	
	int i = SCORE_TXT_MAX - 2;
	while( i > 0){
		Game.score_text[i--] = '0' + ( temp % 10) ; 
		temp /= 10; 
	}	
}

void addParticles( int N, Vector2 pos){ 
	for(int i = 0; i < MAX_PARTICLES; i++){ 
		if( particles[i].life) continue ;  
		particles[i].life = MAX_PART_LIFE; 
		particles[i].vel.x = cos_table[GetRandomValue(0, ROTATIONS)]; 
		particles[i].vel.y = sin_table[GetRandomValue(0, ROTATIONS)]; 
		particles[i].pos   = (Vector2) pos; 
		if( N-- == 0) break;
	}
	return ;
}

void updateParticle( Particle* p, double dt){ 
	if( p->life == 0 ) return; 
	p->pos.x += p->vel.x * dt * 100; 
	p->pos.y += p->vel.y * dt * 100; 

	p->life--; 
	return ; 
}
void renderParticle( Particle* p ){	
	if(!p->life) return; 
	int r = 10 * p->life / MAX_PART_LIFE;
	DrawLineV( p->pos, (Vector2){ 
			p->pos.x - p->vel.x * r, 
			p->pos.y - p->vel.y * r}, WHITE); 
	return;
}

bool checkCollisionPlayer( ) { 
	if( player.state == IMMUNE ) return false; 
	for(int i = 0; i < 3; i++){ 
		for(int x = 0; x < MAX_ASTEROIDS; x++) 
			if( CheckCollisionPointPoly(player.vertices[i], asteroid_pool[x].vertices, MAX_VERTICES))
				return true ;
	}
	return false; 
}

void renderGame( ) { 
	BeginDrawing(); 	
		ClearBackground( BLACK ); 	
		for(int i = 0; i < MAX_BULLETS ; i++){ 
			renderBullet( &bullets[i] );
		}	
		
		for( int i = 0; i < MAX_PARTICLES; i++){ 
			renderParticle( &particles[i]); 
		}
		for(int i = 0; i < MAX_ASTEROIDS; i++){ 
			renderAsteroid( &asteroid_pool[ i ]);
		}
		renderPlayer();
		renderUI() ;
	EndDrawing(); 
	return ; 
}

void updateGame( double delta){ 
	if( Game.current_sound ){ 
		PlaySound( sounds[ Game.current_sound ] ); 
		Game.current_sound = EMPTY ; 

	}
	updatePlayer( delta ); 
	if(checkCollisionPlayer()){ 
		Game.healths -= 1;	
		player.state  = IMMUNE ; 
		player.eff_life = IMMUNE_DURATION ; 
		Game.current_sound = HIT; 
		if( Game.healths < 0){ 
			player.state = DEAD  ; 
			Game.state   = GAMEOVER; 
		}
	}
	for(int i = 0; i < MAX_BULLETS ; i++){ 
		updateBullet( &bullets[i] , delta); 	
	}	
	
	//updates asteroids && check collision 
	for(int i = 0; i < MAX_ASTEROIDS; i++){ 
		updateAsteroid( &asteroid_pool[i] , delta );
		//check collision with bullets
		for(int i = 0; i < MAX_BULLETS; i++){ 
			if( bullets[i].active == false ) continue; 
			for(int j = 0; j < MAX_ASTEROIDS; j++){ 
				if( asteroid_pool[j].active == false) continue; 
				if( CheckCollisionPointPoly( bullets[i].pos, asteroid_pool[j].vertices, MAX_VERTICES )){
					Game.current_sound = EXPLODE ; 
					updateScore( 10 ) ; 
					addParticles( 10, asteroid_pool[j].center); 
					asteroid_pool[j].active = false; 
					bullets[i].active = false;

					
					if( asteroid_pool[j].size == LARGE) 
						initDebris( asteroid_pool[j].center ); 
					else if( asteroid_pool[j].size == SMALL) { 
						initAestroid( &asteroid_pool[j]);
					}
					break; 
				}
			}
		}			
	}

	for( int i = 0; i < MAX_PARTICLES; i++){ 
		updateParticle( &particles[i], delta);
	}
	return ;
}

void renderGameOver() { 	
	DrawText( TEXT_GAME_OVER,  WIDTH / 2 - ( TEXT_SIZE_GAMEOVER / 2 ) , HEIGHT / 2 - FONTSIZE_L, FONTSIZE_L, WHITE ); 
	DrawText ( TEXT_PROMPT_1 , WIDTH / 2 - ( TEXT_SIZE_PROMPT1 /  2 ), HEIGHT / 2 + 200, FONTSIZE_S, WHITE);

	//display the score 
	return ; 
}

void renderMenu () { 
	BeginDrawing() ; 
		ClearBackground( BLACK ); 	
		if( GetRandomValue(0, 1)){
			DrawText ( TEXT_PROMPT_2 , WIDTH / 2 - ( TEXT_SIZE_PROMPT2 /  2 ), HEIGHT / 2, FONTSIZE_L, WHITE);
		}
		else 
			DrawText ( TEXT_PROMPT_2 , WIDTH / 2 - ( TEXT_SIZE_PROMPT2 /  2 ), HEIGHT / 2, FONTSIZE_L, (Color){255, 255, 255, 50}) ; 

	EndDrawing() ; 
	return ; 
}
