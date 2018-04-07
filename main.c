#include <stdio.h>
#include <SDL.h>
#include <input.h>
#include <graphic.h>
#include <collider.h>
#include <malloc.h>
#include <mem.h>

const int SCREEN_WIDTH = 640;
const int SCREEN_HEIGHT = 480;
const int SPEED = 300;
const int FPS = 60;

typedef struct animation{
    SDL_Rect *sprites;
    int frame;
    float duration;
    float count;
    float length;
    bool_t onLoop;
} animation_t;

typedef struct vector2 {
    float x, y;
} vector2_t;

typedef struct entity {
    animation_t currentAnimation;
    vector2_t position;
    vector2_t velocity;
    SDL_Rect srcR;
    SDL_Rect destR;
    SDL_Texture* texture;
    SDL_Rect *spriteSheet;

} entity_t;

SDL_Rect *slice_array(SDL_Rect *array, int start, int end) {
    int numElements = (end - start + 1);
    size_t numBytes = sizeof(SDL_Rect) * numElements;

    SDL_Rect *slice = malloc(numBytes);
    memcpy(slice, array + start, numBytes);

    return slice;
}

animation_t newAnimation(SDL_Rect *spriteSheet, int startFrame, int endFrame,float duration, bool_t onLoop){
    animation_t resultAnimation;

    resultAnimation.sprites = slice_array(spriteSheet, startFrame, endFrame);
    resultAnimation.count = 0;
    resultAnimation.frame = -1;
    resultAnimation.length = endFrame - startFrame + 1;
    resultAnimation.duration = duration;
    resultAnimation.onLoop = onLoop;

    return resultAnimation;
}

void playAnimation(entity_t *entity, animation_t *animation, float dt){
    animation->count += dt;
    if(animation->count >= animation->duration){
        animation->count = 0;
        animation->frame++;

        if(animation->frame >= animation->length)
            animation->frame = (animation->onLoop) ? 0 : animation->frame-1;

        entity->srcR = animation->sprites[animation->frame];
    }
}

int main(int argc, char* args[]){
    char gameTitle[] = "Game Window";
    SDL_Window* window = NULL;
    SDL_Renderer* renderer = NULL;
    int close_requested = 0;

    int prevTime = 0;
    int currentTime = 0;
    float deltaTime = 0;

    entity_t player, enemy;
    input_state_t inputState = {};

    if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_TIMER) != 0) {
        printf("error initializing SDL: %s\n", SDL_GetError());
        return 1;
    }

    window = SDL_CreateWindow(gameTitle, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, 0);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    //Player Data
    player.texture = loadTexture(renderer, "content/oldHero.png");
    SDL_QueryTexture(player.texture, NULL, NULL, &player.srcR.w, &player.srcR.h);
    player.destR.w = 16 * 4;
    player.destR.h = 18 * 4;
    player.position.x = 40;
    player.position.y = (SCREEN_HEIGHT - player.destR.h);

    player.spriteSheet = splitImage(&player.srcR, 6, 1);
    player.currentAnimation = newAnimation(player.spriteSheet, 0, 5, 0.1f, TRUE);

    //Enemy Data
    enemy.texture = loadTexture(renderer, "content/image.png");
    SDL_QueryTexture(enemy.texture, NULL, NULL, &enemy.destR.w, &enemy.destR.h);
    enemy.destR.w /= 4;
    enemy.destR.h /= 4;
    enemy.destR.x = (SCREEN_WIDTH - enemy.destR.w);
    enemy.destR.y = (SCREEN_HEIGHT - enemy.destR.h);

    while(!close_requested) {
        //Time loop
        prevTime = currentTime;
        currentTime = SDL_GetTicks();
        deltaTime = (currentTime - prevTime) / 1000.0f;

        updateInput(&inputState);

        player.velocity.x = player.velocity.y = 0;

        //Check inputs
        if(inputState.esc == PRESSED) close_requested = 1;
        if(inputState.up == PRESSED) player.velocity.y += -SPEED;
        if(inputState.down == PRESSED) player.velocity.y += SPEED;
        if(inputState.left == PRESSED) player.velocity.x += -SPEED;
        if(inputState.right == PRESSED) player.velocity.x += SPEED;

        //Collision detection
        if(boxCollision(player.destR, enemy.destR))
            SDL_SetTextureColorMod(player.texture, 255, 0, 0);
        else
            SDL_SetTextureColorMod(player.texture, 255, 255, 255);

        //Update entity velocity
        player.position.x += player.velocity.x * deltaTime;
        player.position.y += player.velocity.y * deltaTime;

        //Window collision detection
        if(player.position.x <= 0) player.position.x = 0;
        if(player.position.y <= 0) player.position.y = 0;
        if(player.position.x >= SCREEN_WIDTH - player.destR.w) player.position.x = SCREEN_WIDTH - player.destR.w;
        if(player.position.y >= SCREEN_HEIGHT - player.destR.h) player.position.y = SCREEN_HEIGHT - player.destR.h;

        //Apply animation
        playAnimation(&player, &player.currentAnimation, deltaTime);

        //Update entity position
        player.destR.x = (int) player.position.x;
        player.destR.y = (int) player.position.y;

#if DEBUG
        //Set background color
        SDL_SetRenderDrawColor( renderer, 0xFF, 0xFF, 0xFF, 0xFF );
        SDL_RenderClear(renderer);

        //Box collider render
        SDL_SetRenderDrawColor( renderer, 0xFF, 0x00, 0x00, 0xFF );
        SDL_RenderDrawRect( renderer, &enemy.destR );
        SDL_SetRenderDrawColor( renderer, 0xFF, 0x00, 0x00, 0xFF );
        SDL_RenderDrawRect( renderer, &player.destR );
#endif

        //Entity renders
        SDL_RenderCopy(renderer, enemy.texture, NULL, &enemy.destR); // Depth 0
        SDL_RenderCopy(renderer, player.texture, &player.srcR, &player.destR); // Depth 1
        SDL_RenderPresent(renderer);

        if(currentTime < FPS)
            SDL_Delay(currentTime - (Uint32)prevTime);
    }

    SDL_DestroyTexture(player.texture);
    SDL_DestroyTexture(enemy.texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    SDL_Quit();
    return 0;
}