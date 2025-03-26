#pragma once

// Si clock_nanosleep n'est pas disponible (MacOS), décommenter la ligne suivante
// #define MACOS_SLEEP

// Commenter cette ligne pour retirer les couleurs
#define COLOR_PRINT

#ifdef COLOR_PRINT
    // Affiche le texte donné en vert
    #define printf_green(...) (printf("\033[0;32m"),printf(__VA_ARGS__),printf("\033[0m"))
    // Affiche le texte donné en rouge
    #define printf_red(...) (printf("\033[0;31m"),printf(__VA_ARGS__),printf("\033[0m"))
#else
    #define printf_green(...) printf(__VA_ARGS__)
    #define printf_red(...) printf(__VA_ARGS__)
#endif

#define NUM_PARTS 8

// Période du tapis roulant. Il avance d'une position toutes les BELT_PERIOD ms
#define BELT_PERIOD 1000 // ms

// Délai minimum pour une installation par un bras robot
#define MIN_DELAY 50
// Délai maximum pour une installation par un bras robot
#define MAX_DELAY 300

// Nombre maximum de positions sur le tapis roulant
#define MAX_POSITION (NUM_PARTS+1)

// Probabilité de blocage d'une installation par un bras robot
// Sous la forme 1/ONE_IN_BLOCK_CHANCE
#define ONE_IN_BLOCK_CHANCE 100

// Liste des erreurs pouvant être retournées par les différentes fonctions
typedef enum {
    // Pas d'erreur
    OK = 0,
    // Dépendances non satisfaites pour l'installation
    INSTALL_REQUIREMENTS = 1,
    // La ligne de production est en fonctionnement
    LINE_STARTED = 2,
    // La position pour l'installation est incorrecte
    INCORRECT_POSITION = 3,
    // La position de la voiture sur le tapis roulant est incorrecte
    INCORRECT_BELT_POSITION = 4,
    // La position où on essaie d'ajouter un bras robot est déjà occupée
    NON_EMPTY_POSITION = 5,
    // Erreur pour obtenir le temps (clock_gettime)
    TIME_ERROR = 6,
    // Erreur de sémaphore (sem_wait, ...)
    SEM_ERROR = 7,
    // Erreur de malloc
    MALLOC_ERROR = 8,
    // La ligne de production est à l'arret
    LINE_STOPPED = 9,
    // Pointeur invalide
    INVALID_POINTER = 10
} error_t;

// Liste des parties de la voiture à installer
typedef enum {
    // Le chassis
    PART_FRAME = 0,
    // Le moteur
    PART_ENGINE = 1,
    // Les roues
    PART_WHEELS = 2,
    // La carosserie
    PART_BODY = 3,
    // Les portières
    PART_DOORS = 4,
    // Les fenêtes
    PART_WINDOWS = 5,
    // Les phares
    PART_LIGHTS = 6,
    // Rien
    PART_EMPTY = 7
} part_t;

// Cotés de la ligne d'assemblage
typedef enum {
    LEFT=0,
    RIGHT=1,
} side_t;

// Type à utiliser pour la ligne d'assemblage
typedef struct assembly_line *assembly_line_t;

/**
 * Initialise et aloue dynamiquement une ligne d'assemblage
 *
 * @param line un pointeur vers une valeur de type assembly_line_t
 *
 * @return un code d'erreur :
 *     - OK si tout s'est bien passé
 *     - MALLOC_ERROR si la mémoire n'a pas pu être allouée
 *     - SEM_ERROR si la création des sémaphores a echoué
 */
error_t init_assembly_line(assembly_line_t *line);

/**
 * Libère les ressources utilisées par la ligne d'assemblage
 *
 * @param line un pointeur vers une valeur de type assembly_line_t
 * @return un code d'erreur :
 *     - OK si tout s'est bien passé
 *     - SEM_ERROR si la destruction des sémaphores a echoué
 */
error_t free_assembly_line(assembly_line_t *line);

/**
* Configure un bras robot qui installe la partie donnée du côté side à la
* position donée.
* @param line la ligne d'assemblage
* @param part la partie de la voiture
* @param side le côté de la ligne d'assemblage (on peut avoir jusqu'à un bras de
* chaque côté pour une position donnée)
* @param position la position sur la ligne (entre 1 et MAX_POSITION)
*
* @return un code d'erreur :
*     - OK si tout s'est bien passé
*     - LINE_STARTED si la ligne d'assemblage est en cours de fonctionnement
*     - INCORRECT_POSITION si la position est incorrecte
*     - NON_EMPTY_POSITION si la position est déjà occupée
*/
error_t setup_arm(assembly_line_t line, part_t part, side_t side, unsigned int position);

/**
 * Démarre la ligne d'assemblage.
 *
 * Le tapis roulant avancera d'une position toutes les BELT_PERIOD ms. Quand le
 * dernier bras robot sera passé, on attends BELT_PERIOD teste si la voiture est
 * correcte et on affiche un message en console.
 *
 * Après le test, on attends encore BELT_PERIOD avant de retourner en position
 * zéro et d'initialiser une nouvelle voiture. Le processus recommence ensuite.
 *
 * La ligne atteindra la position 1 (premier bras robot) après BELT_PERIOD ms.
 *
 * @param line la ligne d'assemblage
 *
 * @return un code d'erreur :
 *     - OK si tout s'est bien passé
 *     - LINE_STARTED si la ligne d'assemblage est en cours de fonctionnement
 *     - TIME_ERROR si la récupération du temps a echoué
 */
error_t run_assembly(assembly_line_t line);

/**
 * Indique au bras robot à la position et de ce côté d'effectuer l'installation
 * de son composant.
 *
 * L'installation ne peut être effectuée que si la voiture est à la position
 * donnée et si les composants nécessaires ont déjà été installés.
 *
 * @param line la ligne d'assemblage
 * @param side le côté où se trouve le bras robot
 * @param position la position du bras robot (entre 1 et MAX_POSITION)
 *
 * @return un code d'erreur :
 *     - OK si tout s'est bien passé
 *     - LINE_STOPPED si la ligne d'assemblage est à l'arret
 *     - INCORRECT_POSITION si la position est incorrecte
 *     - INCORRECT_BELT_POSITION si la position de la voiture sur le tapis roulant est incorrecte
 *     - INSTALL_REQUIREMENTS si les composants nécessaires n'ont pas été installés
 */
error_t trigger_arm(assembly_line_t line, side_t side, unsigned int position);

/**
 * Stoppe la ligne d'assemblage et débloque le tapis roulant et les bras robots
 * si besoin.
 *
 * Si une voiture était en production, elle sera retirée du tapis roulant. La
 * ligne d'assemblage est remise à zéro mais les bras robots restent configurés.
 *
 * @param line la ligne d'assemblage
 *
 * @return un code d'erreur :
 *     - OK si tout s'est bien passé
 *     - LINE_STOPPED si la ligne d'assemblage est à l'arret
 */
error_t shutdown_assembly(assembly_line_t line);

/**
 * Affiche les statistiques de la ligne d'assemblage.
 *
 * @param line la ligne d'assemblage
 */
void print_assembly_stats(assembly_line_t line);
