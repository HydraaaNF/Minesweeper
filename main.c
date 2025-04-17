#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

//Représente l'état de la partie
struct game {
    int x_size;
    int y_size;
    int mines;
    int** map;//représente les mines et la valeur des cases non mines
    bool** visible;//indique si une case est visible
    bool** flags;//indique la présence d'un drapeau
};
typedef struct game game;

struct config {
    int x_size;
    int y_size;
    int** map;//indique la valeur d'une case si elle est visible
    int** guess;//Pour chaque case {=0 si pas décidé, =1 si safe, =2 si mine}
};
typedef struct config config;

struct arbre_decision{
    int x;
    int y;
    bool terminal;//Indique si l'arbre est une feuille (pas nécessaire mais rend le code lisible)
    struct arbre_decision** choix;//Fils du noeud courant
};
typedef struct arbre_decision arbre_decision;

int min(int, int);
int max(int, int);
int abs(int);
double rand_rate();
int randint(int, int);
int int_width(int);

void print_tabulation(int);
void print_2d_matrix(int**, int, int);
void print_config(config*);
void afficher_arbre(arbre_decision*, int, int);
void print_arbre(arbre_decision*);
void show_game(game*, bool);

config* game_to_config(game*);
config* copy_config(config*);
void liberer_config(config*);
int config_case_score(config*, int, int);
int case_guess_restant(config*, int, int);
bool est_possible(config*);
bool guess_possible(config*, int, int, int);
bool est_significatif(config*, int, int);
void premier_significatif(config*, int*, int*, bool*);

game* new_game(int, int, int, int, int);
int get_n_significatif(game*);
int mines_adjacentes(game*, int, int);
void ia(game*);
void liberer_game(game*);
void put_flag(game*, int, int, bool*);
void remove_flag(game*, int, int, bool*);
void reveal(game*, int, int, bool*);
bool is_win(game*);
bool is_fail(game*);

arbre_decision* creer_arbre_decision(int, int, bool, arbre_decision*, arbre_decision*);
void liberer_arbre_decision(arbre_decision*);
arbre_decision* rec_decision(config*, int, int, int*);
arbre_decision* decision(config*, int*);
int terminaux(arbre_decision*);
void rec_get_hitmap(arbre_decision*, int**, int);
void get_hitmap(arbre_decision*, int**);


int min(int x, int y){
    //Cette fonction renvoie le minimum entre x et y
    if(x < y){
        return x;
    }
    return y;
}

int max(int x, int y){
    //Cette fonction renvoie le maximum entre x et y
    if(x > y){
        return x;
    }
    return y;
}

int abs(int x){
    //Cette fonction renvoie la valeur absolue de x
    if(x >= 0){
        return x;
    }
    return -x;
}

void print_tabulation(int n){
    //Affiche n espaces d'affilé
    for(int i = 0; i < n; i++){
        printf(" ");
    }
}

void print_2d_matrix(int** mat, int n, int m){
    //Affiche le contenu d'une matrice 2d
    for(int i = 0; i < n; i++){
        for(int j = 0; j < m; j++){
            printf("%i", mat[i][j]);
        }
        printf("\n");
    }
}

config* game_to_config(game* g){
    //Convertit l'état de la partie en une configuration
    config* c = malloc(sizeof(config));
    //mêmes dimensions
    c->x_size = g->x_size;
    c->y_size = g->y_size;
    c->map = malloc(sizeof(int*)*c->y_size);
    c->guess = malloc(sizeof(int*)*c->y_size);
    for(int y = 0; y < g->y_size; y++){
        c->map[y] = malloc(sizeof(int)*c->x_size);
        c->guess[y] = malloc(sizeof(int)*c->x_size);
        for(int x = 0; x < g->x_size; x++){
            c->guess[y][x] = g->flags[y][x] ? 2 : 0;//Les actuels drapeaux sont supposé cacher des mines
            if(g->visible[y][x]){
                c->map[y][x] = g->map[y][x];
            }else{
                c->map[y][x] = -1;
            }
        }
    }
    return c;
}

config* copy_config(config* c){
    //Permet de copier une configuration (pour éviter les effets de bord)
    config* nc = malloc(sizeof(config));
    nc->x_size = c->x_size;
    nc->y_size = c->y_size;
    nc->map = malloc(sizeof(int*)*c->y_size);
    nc->guess = malloc(sizeof(bool*)*c->y_size);
    for(int y = 0; y < c->y_size; y++){
        nc->map[y] = malloc(sizeof(int)*c->x_size);
        nc->guess[y] = malloc(sizeof(int)*c->x_size);
        for(int x = 0; x < c->x_size; x++){
            nc->map[y][x] = c->map[y][x];
            nc->guess[y][x] = c->guess[y][x];
        }
    }
    return nc;
}

void print_config(config* c){
    //Affiche une configuration
    for(int y = 0; y < c->y_size; y++){
        for(int x = 0; x < c->x_size; x++){
            printf("%i", c->guess[y][x]);
        }
        printf("\n");
    }
}

void liberer_config(config* c){
    //Libère une configuration
    for(int y = 0; y < c->y_size; y++){
        free(c->map[y]);
        free(c->guess[y]);
    }
    free(c->map);
    free(c->guess);
    free(c);
}

arbre_decision* creer_arbre_decision(int x, int y, bool terminal, arbre_decision* choix1, arbre_decision* choix2){
    //Creer un objet arbre de décision
    arbre_decision* ad = malloc(sizeof(arbre_decision));
    ad->x = x;
    ad->y = y;
    ad->terminal = terminal;
    ad->choix = malloc(sizeof(arbre_decision*)*2);
    ad->choix[0] = choix1;
    ad->choix[1] = choix2;
    return ad;
}

int config_case_score(config* c, int x, int y){
    //Retourne le score d'une case d'une configuration
    if(c->map[y][x] != -1){//n'a de sens que pour une case visible
        int score = 0;
        if(x > 0){
            if(y>0 && c->guess[y-1][x-1] == 2)score++;
            if(c->guess[y][x-1] == 2)score++;
            if(y<c->y_size-1 && c->guess[y+1][x-1] == 2)score++;
        }
        if(x < c->x_size-1){
            if(y>0 && c->guess[y-1][x+1] == 2)score++;
            if(c->guess[y][x+1] == 2)score++;
            if(y<c->y_size-1 && c->guess[y+1][x+1] == 2)score++;
        }
        if(y>0 && c->guess[y-1][x] == 2)score++;
        if(y<c->y_size-1 && c->guess[y+1][x] == 2)score++;
        return score;
    }else{
        return -1;//On renvoie une valeur impossible si la case est non visible
    }
}

int case_guess_restant(config* c, int x, int y){
    //Retourne le nombre de cases non spéculé autour de la case en entrée
    if(c->map[y][x] != -1){
        int n = 0;
        if(x > 0){
            if(y>0 && c->map[y-1][x-1] == -1 && c->guess[y-1][x-1] == 0)n++;
            if(c->map[y][x-1] == -1 && c->guess[y][x-1] == 0)n++;
            if(y<c->y_size-1 && c->map[y+1][x-1] == -1 && c->guess[y+1][x-1] == 0)n++;
        }
        if(x < c->x_size-1){
            if(y>0 && c->map[y-1][x+1] == -1 && c->guess[y-1][x+1] == 0)n++;
            if(c->map[y][x+1] == -1 && c->guess[y][x+1] == 0)n++;
            if(y<c->y_size-1 && c->map[y+1][x+1] == -1 && c->guess[y+1][x+1] == 0)n++;
        }
        if(y>0 && c->map[y-1][x] == -1 && c->guess[y-1][x] == 0)n++;
        if(y<c->y_size-1 && c->map[y+1][x] == -1 && c->guess[y+1][x] == 0)n++;
        return n;
    }else{
        return -1;
    }
}

bool est_possible(config* c){
    //Indique si une configuration finale est possible
    for(int y = 0; y < c->y_size; y++){
        for(int x = 0; x < c->x_size; x++){
            if(c->map[y][x] > 0 && c->map[y][x] != config_case_score(c, x, y)){//incohérence détecté
                return false;
            }
        }
    }
    return true;
}

bool guess_possible(config* c, int x, int y, int guess){
    //Indique si une spéculation rend la configuration possible
    c->guess[y][x] = guess;
    int ymin = max(0, y-1);
    int xmin = max(0, x-1);
    int ymax = min(c->y_size, y+2);
    int xmax = min(c->x_size, x+2);
    for(int a = ymin; a < ymax; a++){
        for(int b = xmin; b < xmax; b++){
            if((a != y || b != x) && c->map[a][b] > 0){
                if(config_case_score(c, b, a) > c->map[a][b] || config_case_score(c, b, a) + case_guess_restant(c, b, a) < c->map[a][b]){//Case incohérente détecté
                    c->guess[y][x] = 0;
                    return false;
                }
                
            }
        }
    }
    c->guess[y][x] = 0;
    return true;
}

void afficher_arbre(arbre_decision* ad, int profondeur, int indent){
    //Affiche récursivement un arbre de décision
    if(ad!=NULL){
        print_tabulation(indent * profondeur);
        printf("(%i %i)\n", ad->x, ad->y);
        if(!ad->terminal){
            afficher_arbre(ad->choix[0], profondeur+1, indent);
            afficher_arbre(ad->choix[1], profondeur+1, indent);
        }
    }else{
        print_tabulation(indent * profondeur);
        printf("null\n");
    }
}

void print_arbre(arbre_decision* ad){
    //Appel la procédure précédente avec les bonnes valeurs par défaut
    printf("==ARBRE_DECISION==\n");
    afficher_arbre(ad, 0, 3);
    printf("\n\n\n");
}

void liberer_arbre_decision(arbre_decision* ad){
    //Libère un arbre de décision
    if(ad != NULL){
        liberer_arbre_decision(ad->choix[0]);//si le fils est null alors il ne se passera rien dans l'appel récursif
        liberer_arbre_decision(ad->choix[1]);//même chose
        free(ad->choix);
        free(ad);
    }
}

bool est_significatif(config* c, int x, int y){
    //indique si une case est significative (voir rapport)
    if(c->map[y][x] == -1){
        if(x > 0){
            if(y > 0 && c->map[y-1][x-1] != -1)return true;
            if(c->map[y][x-1] != -1)return true;
            if(y < c->y_size-1 && c->map[y+1][x-1] != -1)return true;
        }
        if(x < c->x_size-1){
            if(y > 0 && c->map[y-1][x+1] != -1)return true;
            if(c->map[y][x+1] != -1)return true;
            if(y < c->y_size-1 && c->map[y+1][x+1] != -1)return true;
        }
        if(y > 0 && c->map[y-1][x] != -1)return true;
        if(y < c->y_size-1 && c->map[y+1][x] != -1)return true;
    }
    return false;
}

void premier_significatif(config* c, int* px, int* py, bool* err){
    //Retourne la première case significative trouvé
    bool fin = false;
    *err = true;//dans le cas où rien n'est trouvé
    for(int y = 0; y < c->y_size; y++){
        for(int x = 0; x < c->x_size; x++){
            if(c->map[y][x] == -1 && c->guess[y][x] == 0 && est_significatif(c, x, y)){
                *px = x;
                *py = y;
                *err = false;
                fin = true;//pour sortir de la seconde boucle
                break;
            }
        }
        if(fin){
            break;
        }
    }
}

int get_n_significatif(game* g){
    //Retourne de nombre de cases significatives
    config *c = game_to_config(g);
    bool err = false;
    int n = -1;
    int x, y;
    while(!err){
        premier_significatif(c, &x, &y, &err);
        n++;
        if(!err){
            c->guess[y][x] = 1;
        }
    }
    liberer_config(c);
    return n;
}

arbre_decision* rec_decision(config* c, int x, int y, int* n_possibles){
    //Construit récursivement un arbre de décision
    int nx, ny;
    bool err = false;
    premier_significatif(c, &nx, &ny, &err);
    if(err){//Cas où il n'y a plus de case significative
        *n_possibles += 1;
        return creer_arbre_decision(x, y, true, NULL, NULL);
    }
    arbre_decision* gauche = NULL;
    arbre_decision* droite = NULL;
    if(guess_possible(c, nx, ny, 1)){//Possible de considérer la case comme non mine
        c->guess[ny][nx] = 1;
        gauche = rec_decision(c, nx, ny, n_possibles);
    }
    if(guess_possible(c, nx, ny, 2)){//Possible de considérer la case comme une mine
        c->guess[ny][nx] = 2;
        droite = rec_decision(c, nx, ny, n_possibles);
    }
    c->guess[ny][nx] = 0;
    if(gauche != NULL || droite != NULL){//Cas où la configuration courante mène à une configuration possible
        return creer_arbre_decision(x, y, false, gauche, droite);
    }else{
        /*if(gauche != NULL)liberer_arbre_decision(gauche);
        if(droite != NULL)liberer_arbre_decision(droite);*/
        return NULL;
    }
}

arbre_decision* decision(config* c , int* n_possibles){
    //Appelle la fonction précédente avec les bon paramètres
    return rec_decision(c, -1, -1, n_possibles);
}

double rand_rate(){
    //Renvoie un flottant aléatoire entre 0 et 1
    return (double) rand() / RAND_MAX;
}

int randint(int a, int b){
    //Renvoie un entier aléatoire entre a et b
    return a + (int) (rand_rate()*(b-a));
}

int mines_adjacentes(game* g, int x, int y){
    //Retourne le nombre de mines adjacentes à la case en entrée
    int total = 0;
    if(x >= g->x_size || y >= g->y_size || x < 0 || y < 0){
        return -1;
    }
    if(x > 0){
        if(y > 0 && g->map[y-1][x-1] == -1)total++;
        if(g->map[y][x-1] == -1)total++;
        if(y < g->y_size-1 && g->map[y+1][x-1] == -1)total++;
    }
    if(x < g->x_size-1){
        if(y > 0 && g->map[y-1][x+1] == -1)total++;
        if(g->map[y][x+1] == -1)total++;
        if(y < g->y_size-1 && g->map[y+1][x+1] == -1)total++;
    }
    if(y>0 && g->map[y-1][x] == -1)total++;
    if(y < g->y_size-1 && g->map[y+1][x])total++;
    return total;
}

int terminaux(arbre_decision* ad){
    //Retourne le nombre de feuilles de l'arbre en entrée
    if(ad != NULL){
        if(ad->terminal){
            return 1;
        }
        return terminaux(ad->choix[0]) + terminaux(ad->choix[1]);
    }
    return 0;
}

void rec_get_hitmap(arbre_decision* ad, int** hitmap, int guess){
    //Construit la hitmap
    if(ad != NULL){
        if(ad->x != -1 && ad->y != -1){
            hitmap[ad->y][ad->x] += guess * terminaux(ad);//Mise a jour de la hitmap
        }
        rec_get_hitmap(ad->choix[0], hitmap, -1);//Fils gauche (pas de mine)
        rec_get_hitmap(ad->choix[1], hitmap, 1);//Fils droit (mine)
    }
}

void get_hitmap(arbre_decision* ad, int** hitmap){
    //Appelle la procédure précédente avec les bons paramètres
    rec_get_hitmap(ad, hitmap, 0);
}

void ia(game* g){
    //Affiche le meilleur coup à jouer
    int n_possibles = 0;
    int** hitmap = malloc(sizeof(int*)*g->y_size);
    for(int y = 0; y < g->y_size; y++){
        hitmap[y] = malloc(sizeof(int)*g->x_size);
        for(int x = 0; x < g->x_size; x++){
            hitmap[y][x] = 0;
        }
    }
    config* c = game_to_config(g);
    arbre_decision* ad = decision(c, &n_possibles);
    get_hitmap(ad, hitmap);
    int best_x =0;
    int best_y = 0;
    for(int y = 0; y < c->y_size; y++){
        for(int x = 0; x < c->x_size; x++){
            if(!g->visible[y][x]){
                if(abs(hitmap[y][x]) > abs(hitmap[best_y][best_x])){
                    best_x = x;
                    best_y = y;
                }
            }
        }
    }
    float fiabilite = (abs(hitmap[best_y][best_x]) + n_possibles) / (float) (n_possibles * 2);
    printf("[HELP] %s %i %i (fiabilite %f)\n", hitmap[best_y][best_x] >= 0 ? "flag" : "reveal", best_x, best_y, fiabilite);
    liberer_arbre_decision(ad);
    liberer_config(c);
    for(int y = 0; y < g->y_size; y++){
        free(hitmap[y]);
    }
    free(hitmap);
}

game* new_game(int x_size, int y_size, int mines, int x, int y){
    //Retourne une instance de la structure game
    game* g = malloc(sizeof(game));
    g->x_size = x_size;
    g->y_size = y_size;
    g->mines = mines;
    g->map = malloc(sizeof(int*)*y_size);
    g->visible = malloc(sizeof(bool*)*y_size);
    g->flags = malloc(sizeof(bool*)*y_size);
    for(int i = 0; i < y_size; i++){
        g->map[i] = malloc(sizeof(int)*x_size);
        g->visible[i] = malloc(sizeof(bool)*x_size);
        g->flags[i] = malloc(sizeof(bool)*x_size);
        for(int j = 0; j < x_size; j++){
            g->map[i][j] = 0;
            g->visible[i][j] = false;
            g->flags[i][j] = false;
        }
    }
    int mines_planted = 0;
    int xm, ym;
    while(mines_planted < mines){
        xm = randint(0, g->x_size);
        ym = randint(0, g->y_size);
        if(g->map[ym][xm] != -1 && (abs(x - xm) >= 2 || abs(y - ym) >= 2)){
            g->map[ym][xm] = -1;
            mines_planted++;
        }
    }
    for(int y = 0; y < g->y_size; y++){
        for(int x = 0; x < g->x_size; x++){
            if(g->map[y][x] != -1){
                g->map[y][x] = mines_adjacentes(g, x, y);
            }
        }
    }
    return g;
}

void liberer_game(game* g){
    //Libère une instance de la structure game
    for(int y = 0; y < g->y_size; y++){
        free(g->map[y]);
        free(g->flags[y]);
        free(g->visible[y]);
    }
    free(g->map);
    free(g->flags);
    free(g->visible);
    free(g);
}

void put_flag(game* g, int x, int y, bool* err){
    //Place un drapeau
    if(x >= 0 && x < g->x_size && y >= 0 && y < g->y_size &&!g->flags[y][x] && !g->visible[y][x]){
        //Seulement si les coordonnées sont valide, que la case est invisible et qu'il n'y a pas déjà un drapeau
        g->flags[y][x] = true;
    }else{
        *err = true;
    }
}

void remove_flag(game* g, int x, int y, bool* err){
    if(x >= 0 && x < g->x_size && y >= 0 && y < g->y_size && g->flags[y][x]){
        //Seulement si les coordonnées sont valide, que la case est invisible et qu'il a un drapeau
        g->flags[y][x] = false;
    }else{
        *err = true;
    }
}

void reveal(game* g, int x, int y, bool* err){
    //Révèle récursivement de proche en proche les cases autour de la case en entrée
    *err = false;
    bool autre = false;
    if(x >= 0 && x < g->x_size && y >= 0 && y < g->y_size && !g->visible[y][x]){
        g->visible[y][x] = true;
        if(g->map[y][x] == 0){
            if(x > 0)reveal(g, x-1, y, &autre);
            if(x < g->x_size-1)reveal(g, x+1, y, &autre);
            if(y > 0)reveal(g, x, y-1, &autre);
            if(y < g->y_size-1)reveal(g, x, y+1, &autre);
            if(x > 0 && y > 0 && (g->map[y-1][x] == 0 || g->map[y][x-1] == 0 || g->map[y-1][x-1] > 0))reveal(g, x-1, y-1, &autre);
            if(x > 0 && y < g->y_size-1 && (g->map[y+1][x] == 0 || g->map[y][x-1] == 0 || g->map[y+1][x-1] > 0))reveal(g, x-1, y+1, &autre);
            if(x < g->x_size-1 && y > 0 && (g->map[y-1][x] == 0 || g->map[y][x+1] == 0 || g->map[y-1][x+1] > 0))reveal(g, x+1, y-1, &autre);
            if(x < g->x_size-1 && y < g->y_size-1 && (g->map[y+1][x] == 0 || g->map[y][x+1] == 0 || g->map[y+1][x+1] > 0))reveal(g, x+1, y+1, &autre);
        }
    }else{
        *err = true;
    }
}

bool is_win(game* g){
    //Indique si la partie est gagnée
    for(int y = 0; y < g->y_size; y++){
        for(int x = 0; x < g->x_size; x++){
            if(g->map[y][x] != -1 && !g->visible[y][x]){
                return false;
            }
        }
    }
    return true;
}

bool is_fail(game* g){
    //Indique si la partie est perdu
    for(int y = 0; y < g->y_size; y++){
        for(int x = 0; x < g->x_size; x++){
            if(g->visible[y][x] && g->map[y][x] == -1){
                return true;
            }
        }
    }
    return false;
}

int int_width(int n){
    //Renvoei la largeur d'affichage d'un entier positif
    if(n==0){
        return 1;
    }
    return log10(n)+1;
}

void show_game(game* g, bool debug){
    //Affiche l'état actuel de la partie, quand debug=true, on affiche aussi les cases non visibles
    printf("%i x %i and %i mines\n", g->x_size, g->y_size, g->mines);
    int marge_x = 1+log10(g->x_size);
    int marge_y = 1+log10(g->y_size);
    print_tabulation(marge_y);
    for(int x = 0; x < g->x_size; x++){
        printf("|");
        printf("%i", x);
        print_tabulation(marge_x - int_width(x));
    }
    printf("\n");
    for(int x = 0; x < g->x_size * (marge_x+1) + 1; x++){
        printf("_");
    }
    printf("\n");
    for(int y = 0; y < g->y_size; y++){
        printf("%i", y);
        print_tabulation(marge_y - int_width(y));
        printf("|");
        for(int x = 0; x < g->x_size; x++){
            if(g->visible[y][x] || debug){
                if(g->map[y][x] > 0){
                    printf("%c", '0' + g->map[y][x]);
                }else if(g->map[y][x] == 0){
                    printf(" ");
                }else{
                    printf("X");
                }
            }else if(g->flags[y][x]){
                printf("F");
            }else{
                printf("#");
            }
            print_tabulation(marge_x);
        }
        printf("\n");
    }
    printf("\n\n");
}

int main(int argc, char** argv){
    if(argc == 4){
        int x,y;
        char type[100];
        char mode[100];
        bool err = false;
        bool fin = false;
        int seed = time(NULL);
        printf("[SEED] %i\n", seed);
        printf("Choisissez un mode de génération (random/seed)>");
        scanf("%s", mode);
        printf("\n");
        if(strcmp(mode, "seed")==0){
            printf("Choisissez la graine de génération>");
            scanf("%i", &seed);
            printf("\n");
        }
        srand(seed);
        printf("Choisissez une action à faire\n");
        scanf("%32s %i %i", type, &x, &y);
        game* g = new_game(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), x, y);//génération du terrain
        if(strcmp(type, "reveal")==0){
            reveal(g, x, y, &err);
        }else{
            printf("Erreur commande\n");
            return 1;
        }
        if(err){
            printf("erreur commande\n");
            return 1;
        }
        err = false;
        show_game(g, false);
        do{//boucle principale
            if(is_win(g) || is_fail(g))break;
            err = false;
            printf("Choisissez une action à faire\n");
            scanf("%32s %i %i", type, &x, &y);
            if(strcmp(type, "reveal") == 0){//révéler une case
                reveal(g, x, y, &err);
            }else if(strcmp(type, "flag") == 0){//placer un drapeau
                put_flag(g, x, y, &err);
            }else if(strcmp(type, "unflag") == 0){//retirer un drapeau
                remove_flag(g, x, y, &err);
            }else if(strcmp(type, "ia") == 0){//Demande la suggestion de l'ia
                ia(g);
            }else{
                printf("Unrecognised insctruction\n");
            }
            if(err){
                printf("Something went wrong...\n");
            }
            show_game(g, false);
            printf("Votre score est de %i\n", ((int) time(NULL)) - seed);//Affichage du score
        }
        while(!fin);
        printf(is_win(g) ? "Gg\n" : "Too bad bra\n");
        liberer_game(g);
    }else{
        printf("%s <largeur_grille> <hauteur_grille> <mines>\n", argv[0]);
    }
    return 0;
}