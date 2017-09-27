#include <stdlib.h>
#include <stdio.h>
#include "myLib.h"
#include "main.h"
#include "battleMode.h"
#include "BattleMenu.h"
#include "ghostBoss.h"
#include "winScreen.h"
#include "battleSprites.h"
#include "Overworld.h"
#include "sprites.h"
#include "splash.h"
#include "instructions.h"
#include "glow.h"
#include "bansheeBoss.h"
#include "spectreBoss.h"
#include "sound.h"
#include "iAmTheDoctor.h"
#include "SanctuaryGuradian.h"
#include "titleMusic.h"
#include "medley.h"
#include "story1.h"
#include "story2.h"
#include "pause.h"


//----------------------------------------------------------------//
// Game Explanation
//  -defeat all enemies to win
//  -RPG elements include basic attack and special attack (needs charging after use)
//  -each enemy has different pattern - goal figure it out
//
// Cheat
//  -run option (limited use)
//  -flee battle with full health
//  -allows time to figure out the enemy pattern
//
// Possible Bugs
//  -sound sometimes garbles (can't figure it out)
//
// Extra Credit
//  -used function pointers for state machine and enemy attack pattern
//  -messed with background palettes for selection, etc
//
// Comments
//  -Please read instructions and story fully -- it explains a lot
//  -press select in pause screen to restart game
//
//----------------------------------------------------------------//

int main() {
    initialize();
    setupInterrupts();
    setupSounds();
    while(1) {

        oldButtons = buttons;
        buttons = BUTTONS;
        frameCounter++;
        state();
        
    }
    

    return 0;
}

void init() {
    REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | SPRITE_ENABLE;
    REG_BG0CNT = CBB(0) | SBB(31) | BG_SIZE3 | COLOR256;

    frameCounter = 0;


    //Create player sprite object
    player.width = 12;
    player.height = 16;
    player.row = 160/2 + player.height/2;
    player.col = 240/2 - player.width/2;
    player.rdel = 1;
    player.cdel = 1;

    player.aniCounter = 0;
    player.currFrame = 0;
    player.aniState = PLAYERFRONT;
    player.moving = 0;

    //GENERATE Enemy sprites
    srand(2);
    for (int i = 0; i < ENEMIES; i++) {
        MOVOBJ enemy;
        enemy.width = 16;
        enemy.height = 16;
        enemy.bigRow = rand() % 100 + 140;
        enemy.bigCol = rand() % 500;

        enemy.aniCounter = 0;
        enemy.currFrame = 0;
        enemy.active = 1;
        enemy.enemyNum = i;
        
        enemy.enemyType = rand() % 3;
        enemy.aniState = enemy.enemyType + 4;

        enemies[i] = enemy;
    }

    enemiesLeft = ENEMIES;

    hOffOverworld = 120;
    vOffOverworld = 96;

    runCounter = 0;

    goToOverworld();
}

void initialize() {
    REG_DISPCTL = MODE3 | BG2_ENABLE;
    state = splash;
    bgNum = 0;
    playSoundA(titleMusic, TITLEMUSICLEN, TITLEMUSICFREQ, 0);
}


//--------------------STATE Functions------------------------
//Sets up overworld
void goToOverworld() {
    state = overworld;
    stopSoundA();

    REG_DISPCTL = MODE0 | BG0_ENABLE | SPRITE_ENABLE;
    REG_BG0CNT = CBB(0) | SBB(bg0SBB) | BG_SIZE1;
    loadPalette(OverworldPal);
    loadBgTiles(OverworldTiles, OverworldTilesLen, 0);
    loadBgMap(OverworldMap, OverworldMapLen, bg0SBB);

    loadSpritePalette(spritesPal);
    loadSpriteTiles(spritesTiles, spritesTilesLen);

    hideSprites();
    drawPlayer();

    bossHealth = BOSSHP;
    playerHealth = PLAYERHP;

    player.bigRow = player.row + vOffOverworld;
    player.bigCol = player.col + hOffOverworld;

    playSoundA(SanctuaryGuradian, SANCTUARYGURADIANLEN, SANCTUARYGURADIANFREQ, 1);

}

//sets up RPG battle mode
void goToBattle() {
    state = battle;
    stopSoundA();

    REG_DISPCTL = MODE0 | BG0_ENABLE | BG1_ENABLE | BG2_ENABLE | SPRITE_ENABLE;
    REG_BG0CNT = CBB(0) | SBB(bg0SBB) | BG_SIZE3;
    REG_BG1CNT = CBB(1) | SBB(bg1SBB) | BG_SIZE3;
    REG_BG2CNT = CBB(2) | SBB(bg2SBB) | BG_SIZE1;

    REG_BG0HOFS = 0;
    REG_BG0VOFS = 0;

    battleSelect = ATTACK;
    attackCounter = 0;
    specialCounter = 2;

    //Loading battle menu
    loadPalette(BattleMenuPal);


    loadBgTiles(BattleMenuTiles, BattleMenuTilesLen, 0);
    loadBgMap(BattleMenuMap, BattleMenuMapLen, bg0SBB);
    //Changing the palette of the selectors to green
    paletteSwap(11, 8);
    paletteSwap(12, 8);
    paletteSwap(13, 8);


    //Assigns the attack pattern the enemy will use
    //based on the type that the player collided with
    //Assigns approriate enemy background to enemy type
    switch (type) {
        case GHOST:
            loadBgTiles(ghostBossTiles, ghostBossTilesLen, 1);
            loadBgMap(ghostBossMap, ghostBossMapLen, bg1SBB);
            enemyPattern = pattern1;
            break;
        case BANSHEE:
            loadBgTiles(bansheeBossTiles, bansheeBossTilesLen, 1);
            loadBgMap(bansheeBossMap, bansheeBossMapLen, bg1SBB);
            enemyPattern = pattern2;
            break;
        case SPECTRE:
            loadBgTiles(spectreBossTiles, spectreBossTilesLen, 1);
            loadBgMap(spectreBossMap, spectreBossMapLen, bg1SBB);
            enemyPattern = pattern3;
            break;
    }

    //load the glow background
    loadBgTiles(glowTiles, glowTilesLen, 2);
    loadBgMap(glowMap, glowMapLen, bg2SBB);
    //revert glow to invisible
    paletteSwap(50, 51);

    loadSpritePalette(battleSpritesPal);
    loadSpriteTiles(battleSpritesTiles, battleSpritesTilesLen);

    hideSprites();
    drawBossHP(BOSSHP, bossHealth);
    drawPlayerHP(PLAYERHP, playerHealth);

    playSoundA(iAmTheDoctor, IAMTHEDOCTORLEN, IAMTHEDOCTORFREQ, 1);
}

void splash() {
    drawBackgroundImage3(&splashBitmap);

    if (BUTTON_PRESSED(BUTTON_START)) {
        state = instruct;
        playSoundA(medley, MEDLEYLEN, MEDLEYFREQ, 1);
    }
}

void instruct() {
    backgroundSwitch(&bgNum);
    switch(bgNum) {
        case 0:
            drawBackgroundImage3(&instructionsBitmap);
            break;
        case 1:
            drawBackgroundImage3(&story1Bitmap);
            break;
        case 2:
            drawBackgroundImage3(&story2Bitmap);
            break;
    }
    if (BUTTON_PRESSED(BUTTON_START) && bgNum == 2) {
        init();
    }
}

void win() {
    REG_DISPCTL = MODE3 | BG2_ENABLE;
    stopSoundA();
    drawBackgroundImage3(&winScreenBitmap);
    if (BUTTON_PRESSED(BUTTON_START)) {
        initialize();
    }
}

void lose() {
    REG_DISPCTL = MODE3 | BG2_ENABLE;
    stopSoundA();
    fillScreen3(RED);
    if (BUTTON_PRESSED(BUTTON_START)) {
        initialize();
    }
}

void goToPauseOverworld() {
    state = pauseOverWorld;
    REG_DISPCTL = MODE3 | BG2_ENABLE;
    pauseSound();
    drawBackgroundImage3(&pauseBitmap);
}

void pauseOverWorld() {
    if (BUTTON_PRESSED(BUTTON_START)) {
        unpauseSound();
        goToOverworld();
    }

    if (BUTTON_PRESSED(BUTTON_SELECT)) {
        initialize();
    }
}

void goToPauseBattle() {
    state = pauseBattle;
    REG_DISPCTL = MODE3 | BG2_ENABLE;
    pauseSound();
    drawBackgroundImage3(&pauseBitmap);   
}

void pauseBattle() {

    if (BUTTON_PRESSED(BUTTON_START)) {
        unpauseSound();
        goToBattle();
    }
}
//--------------End of STATES--------------------------------------


//-----------------------UPDATE Functions---------------------------------------------
void overworld() {
    if (BUTTON_HELD(BUTTON_UP)) {
        player.aniState = PLAYERBACK;
        if (player.bigRow > 140) {
            if (player.bigRow <= 80 || player.bigRow >= (256 - 80 - player.width)) {
                player.row--;
            } else {
                vOffOverworld--;
            }
        }
    }

    if (BUTTON_HELD(BUTTON_DOWN)) {
        player.aniState = PLAYERFRONT;

        if ((player.bigRow + player.height) < 256) {
            if (player.bigRow <= 80 || player.bigRow >= (256 - 80 - player.width)) {
                player.row++;
            } else {
                vOffOverworld++;
            }
        }
    }

    if (BUTTON_HELD(BUTTON_LEFT)) {
        player.aniState = PLAYERLEFT;
        if (player.bigCol > 7) {
            if (player.bigCol <= 120 || player.bigCol > (512 - 120)) {
                player.col--;
            } else {
                hOffOverworld--;
            }
        }
    }

    if (BUTTON_HELD(BUTTON_RIGHT)) {
        player.aniState = PLAYERRIGHT;

        if ((player.bigCol + player.width) < 512) {
            if (player.bigCol <= 120 || player.bigCol > (512 - 120)) {
                player.col++;
            } else {
                hOffOverworld++;
            }
        }
    }

    REG_BG0HOFS = hOffOverworld;
    REG_BG0VOFS = vOffOverworld;

    player.bigRow = player.row + vOffOverworld;
    player.bigCol = player.col + hOffOverworld;

    //Placing enemies on overworld
    
    MOVOBJ* enemy;
    for (int i = 0; i < ENEMIES; i++) {
        enemy = &enemies[i];
        enemy->row = enemy->bigRow - vOffOverworld;
        enemy->col = enemy->bigCol - hOffOverworld;
        //Hide enemies when off screen
        enemy->hide = (enemy->col < 0 || (enemy->col - enemy->width) > 240) ? 1 : 0;

        //Collison function
        int collision = collisionObjects(&player, enemy);
        if (collision) {
            enemyFought = enemy->enemyNum;
            type = enemy->enemyType;
            goToBattle();
        }
    }
    

    

    if (BUTTON_PRESSED(BUTTON_START)) {
        goToPauseOverworld();
    }

    animate();
    drawPlayer();
    drawEnemies();

    waitForVblank();
}

void battle() {
    //Selects the item in the menu
    menuSelect(&battleSelect);
    drawBossHP(BOSSHP, bossHealth);
    drawPlayerHP(PLAYERHP, playerHealth);



    //Chooses the battle tactic
    if (BUTTON_PRESSED(BUTTON_A)) {
        if (battleSelect == 3 && runCounter < 5) {
            player.row = 160/2 + player.height/2;
            player.col = 240/2 - player.width/2;
            hOffOverworld = 120;
            vOffOverworld = 96;
            runCounter++;
            goToOverworld();
        } else {
            attack(&attackCounter, &specialCounter, &bossHealth, BOSSHP, battleSelect);
            if (bossHealth <= 0) {
                MOVOBJ* enemy;
                enemy = &enemies[enemyFought];
                enemy->active = 0;
                enemiesLeft--;
                goToOverworld();
            } else {
                for (int i = 0; i < 300; i++) {
                    waitForVblank();
                }

                enemyPattern(&attackCounter, &playerHealth, PLAYERHP, battleSelect);
                if (playerHealth <= 0) {
                    state = lose;
                }
            }
        }
    }

    if (!enemiesLeft) {
        state = win;
    }

    if (BUTTON_PRESSED(BUTTON_START)) {
        goToPauseBattle();
    }

    waitForVblank();
}


//---------------------End of Update-------------------------




//---------------------Other Functions-------------------------------------------
void hideSprites() {
    for (int i = 0; i < SHADOWOAM; i++) {
        shadowOAM[i].attr0 = ATTR0_HIDE;
    }
}

int collisionObjects(MOVOBJ* obj1, MOVOBJ* obj2) {
    if (obj2->active) {
        return collisionCheckRect(obj1->row, obj1->col, obj1->height, obj1->width,
                                obj2->row, obj2->col, obj2->height, obj2->width);
    } else {
        return 0;
    }
    
}

void backgroundSwitch(int* bgNum) {
    int num = *bgNum;
    if (BUTTON_PRESSED(BUTTON_LEFT)) {
        if (num) {
            num--;
        } else {
            num = 2;
        }
    }

    if (BUTTON_PRESSED(BUTTON_RIGHT)) {
        if (num == 2) {
            num = 0;
        } else {
            num++;
        }
    }
    *bgNum = num;
}
//---------------------End of Other-----------------------------




//-------------------------------DRAW Functions--------------------------------------
void drawBossHP(int maxHP, int bossHP) {
    for (int i = 0; i < maxHP; i++) {
        shadowOAM[i].attr0 = ATTR0_HIDE;
    }

    for (int i = 0; i < bossHP; i++) {
        shadowOAM[i].attr0 = 0 | ATTR0_4BPP;
        shadowOAM[i].attr1 = i * 15 | ATTR1_SIZE16;
        shadowOAM[i].attr2 = SPRITEOFFSET16(0, i*2);
    }

    DMANow(3, shadowOAM, OAM, SHADOWOAM*4);

    waitForVblank();  
}

void drawPlayerHP(int maxHP, int playerHP) {
    for (int i = 0; i < maxHP; i++) {
        shadowOAM[i + BOSSHP].attr0 = ATTR0_HIDE;
    }

    for (int i = 0; i < playerHP; i++) {
        shadowOAM[i + BOSSHP].attr0 = 144 | ATTR0_4BPP;
        shadowOAM[i + BOSSHP].attr1 = i * 15 | ATTR1_SIZE16;
        shadowOAM[i + BOSSHP].attr2 = SPRITEOFFSET16(2, i*2);
    }

    DMANow(3, shadowOAM, OAM, SHADOWOAM*4);

    waitForVblank();  
}


void drawPlayer() {
    shadowOAM[0].attr0 = player.row | ATTR0_4BPP | ATTR0_SQUARE;
    shadowOAM[0].attr1 = player.col | ATTR1_SIZE16;
    shadowOAM[0].attr2 = SPRITEOFFSET16(player.currFrame*2, player.aniState*2);

    DMANow(3, shadowOAM, OAM, SHADOWOAM*4);

}

void drawEnemies() {
    MOVOBJ* enemy;
    for (int i = 0; i < ENEMIES; i++) {
        enemy = &enemies[i];
        shadowOAM[1 + i].attr0 = enemy->row;
        if (enemy->hide || !enemy->active) {
            shadowOAM[1 + i].attr0 |= ATTR0_HIDE;
        }
        shadowOAM[1 + i].attr1 = enemy->col | ATTR1_SIZE16;
        shadowOAM[1 + i].attr2 = SPRITEOFFSET16(0, enemy->aniState*2);
    }
    

    DMANow(3, shadowOAM, OAM, SHADOWOAM*4);

}

void animate()
{
    if (!player.moving) player.prevAniState = player.aniState;
        
    player.moving = 0;
        
    if(player.aniCounter % 10 == 0) 
    {
        player.aniCounter = 0;
        if (player.currFrame == 3) player.currFrame = 0;
        else player.currFrame++;
    }


    if(BUTTON_HELD(BUTTON_UP))
    {
        player.aniState = PLAYERBACK;
        player.moving = 1;
    }
    if(BUTTON_HELD(BUTTON_DOWN))
    {
        player.aniState = PLAYERFRONT;
        player.moving = 1;
    }
    if(BUTTON_HELD(BUTTON_LEFT))
    {
        player.aniState = PLAYERLEFT;
        player.moving = 1;
    }
    if(BUTTON_HELD(BUTTON_RIGHT))
    {
        player.aniState = PLAYERRIGHT;
        player.moving = 1;
    }
        
    if(!player.moving)
    {
        player.currFrame = 0;
    }
    else
    {
        player.aniCounter++;
    }
}