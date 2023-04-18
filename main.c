#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define DEBUG 0


// Fonction parcourant un arbre binaire de type merkle et affichant les empreintes de chaque noeud de l'arbre par un tableau (parcours en profondeur),
// ici l'arbre est représenté par un tableau d'entiers à deux dimensions (la dimension ici sert pour les empreintes du hash)
void parcoursArbre(int **arbre, int cpt, int nb){
    if (cpt < nb){
        // affichage de l'empreinte
        fprintf(stdout,"noeud [%d]\t\t: ",cpt);
        for (size_t i = 0; i < 5; i++)
            fprintf(stdout,"%d ", arbre[cpt][i]);
        
        fprintf(stdout,"\n");
        parcoursArbre(arbre, cpt*2+1, nb);
        parcoursArbre(arbre, cpt*2+2, nb);
    }
}

// Cette fonction permet à partir de 2 entiers donnés en paramètres de renvoyer un entier avec n bits à 1 à gauche et m bits à 0 à droite
int extraction(int n, int m){
    int num = (1 << n) - 1;
    num = num << m;
    return num;
}

// Fonction permettant le calcul de l'empreinte par l'algorithme TTH(64,5) d'une matrice 'tabMatrice' 5*5 composé de lignes représenté par un entier
void TTH(int *tabMatrice, int *tabEmpreinte){

    int nb = 0;
    int tmp;

    if(DEBUG){
        fprintf(stdout," empreinte : ");
        for (int i = 0; i < 5; i++)
        {
            fprintf(stdout,"%d ",tabEmpreinte[i]);
        }
    }

    // On ajoute à l'empreinte courante la somme de chaque colonne en regard du bloc en cours
    for (int a = 24, j = 0; a >= 0; a-=6, j++){
        nb = 0;
        for (int i = 0; i < 5; i++){
            tmp = tabMatrice[i] & extraction(6,a);
            tmp = tmp >> a;
            nb+=tmp;
        }
        tabEmpreinte[j]= (tabEmpreinte[j]+nb)%64;
    }

    if(DEBUG){
        fprintf(stdout," empreinte : ");
        for (int i = 0; i < 5; i++)
        {
            fprintf(stdout,"%d ",tabEmpreinte[i]);
        }
    }
    
    // On effectue le pivotage du bloc
    int debut = 0,
        fin = 0;
    for (int i = 1, a = 6, b = 24; i < 5; i++, a+=6, b-=6){
        debut = tabMatrice[i] & extraction(a,b);
        fin = tabMatrice[i] & extraction(b,0);
        fin = fin << a;
        debut = debut >> b;
        tabMatrice[i] = fin + debut;
    }

    // On ajoute à nouveau à l'empreinte courante la somme de chaque colonne en regard du bloc en cours
    for (int a = 24, j = 0; a >= 0; a-=6, j++){
        nb = 0;
        for (int i = 0; i < 5; i++){
            tmp = tabMatrice[i] & extraction(6,a);
            tmp = tmp >> a;
            nb+=tmp;
        }
        tabEmpreinte[j]= (tabEmpreinte[j]+nb)%64;
    }

    if(DEBUG){
        fprintf(stdout," empreinte : ");
        for (int i = 0; i < 5; i++)
        {
            fprintf(stdout,"%d ",tabEmpreinte[i]);
        }
    }
    
}


int DataBlockHash(char *message,int **tabEmpreinte, int compteur, int w){
    // Pour construire les matrices 5*5 par bloc de data de 512 octets
        // Correspond au nombre de matrice construite à un instant t
        int nbPetiteMatrice = 0,
        // Correspond au nombre de caractère de lu et ajouté dans une matrice sur les 512 caractères à lire
            nbCarac = 0,
        // Représentation de la matrice 5*5 par 5 entiers int de 32 bits
            matrice[5],
        // On enregistrera dans ce tableau le nombre de bits utilisé par ligne de la matrice (utile à sa construction)
            matriceContenu[5];

    // Variable temporaire permettant le bon déroulement de l'algorithme
        int retenu = 0,     // enregistre les bits qui n'ont pas pu être enregistré en fin de ligne pour les insérer en début de ligne de la matrice (ligne représenté par un entier). Entendre ici "début d'entier pour début de ligne"
            flagRetenu = 0, // si à la prochaine itération il doit y avoir une insertion de bits enregistré dans la variable 'retenu', enregistre également le nombre de bit a utiliser dans la variable 'retenu'
            extraction1 = 0,// permet de stoker des entiers utile pour le calcul bit à bit
            caracTmp = 0;   // enregistre une suite de bit temporaire qu'il faudra par la suite insérer dans les lignes des matrices que l'on va construire

    // Le compteur qui compte le nombre de matrice de 5*5 que contiendra un bloc de 512 octets est initialisé à 0,
    // Cette valeur doit être de 28 en fin d'itération pour chaque bloc de 512 octets :
    // 30 bits utilisé par ligne de bloc, 5 lignes par bloc soit : 5 * 30 bits de données dans une matice 5*5
    // 512 octets = 4096 bits.
    // 4096/(5*30) = 27,30667
    // On aura bien 28 matrices, on pourra bourrer cette dernière matrice par 32 et une suite de 0 (comme ce qu'on a fait pour le message) 
    while(nbPetiteMatrice < 28){
        
        // On va y ici réinitialiser la matrice de 5*5 (nous avons choisi d'utiliser une matrice 5*1 avec un entier qui contient les informations de plusieurs caractères)
        for (int j = 0; j < 5; j++){
            matriceContenu[j] = 0;
            matrice[j] = 0;
        }

        // Construction d'une matrice 5*5, on construit les lignes de cette matrice une après une
        for (int i = 0; i < 5; i++){
            // Pour chaque ligne d'une matrice 
            // Tant que 30 bits n'ont pas été insérés dans la ligne (ligne représenté par un entier int)
            while(matriceContenu[i]<30){
                // Si on se trouve en début de ligne, et que la ligne précédente n'a pas pu insérer tous les bits
                // On les insére ici
                if(flagRetenu){
                    matrice[i] = retenu << (30-flagRetenu);
                    matriceContenu[i]+=flagRetenu;
                    flagRetenu = 0;
                }
                // Si nous n'avons pas assez de place pour insérer les derniers bits dans la ligne,
                // On les garde en mémoire pour les insérer en début de prochaine ligne (ou début de nouvelle matrice)
                if(matriceContenu[i]+ 8 > 30){
                    // Si tous les caractères ont été lus on bourre la dernière matrice qui correspond à la 28ème,
                    // On insère 32 puis des 0 à la suite de ce 32
                    if(nbCarac == 512 && nbPetiteMatrice == 27){
                        caracTmp = 32;
                    }
                    else{
                        if(nbCarac > 512 && nbPetiteMatrice == 27){
                            caracTmp = 0;
                        }
                        // Si ce n'est pas le cas on récupère le dernier caractère à lire
                        else{
                            caracTmp = message[compteur]; 
                            compteur++;
                        }
                    }
                    // Un nouveau caractère a été inséré dans la matrice
                    nbCarac++;

                    // On récupère ici les bits que nous ne pouvons pas insérer dans cette ligne
                    extraction1 = extraction(8-(30-matriceContenu[i]),0);
                    retenu = caracTmp & extraction1;
                    // On indique pour le début de la prochaine ligne, qu'il y aura des bits "retenu" à insérer 
                    // On donne à la variable 'flagRetenu' le nombre de bits à insérer
                    flagRetenu = 8 -(30 - matriceContenu[i]);

                    // On récupère ici les bits que nous pouvons insérer dans cette ligne
                    extraction1 = extraction(30-matriceContenu[i],8-(30-matriceContenu[i]));
                    caracTmp = caracTmp & extraction1;
                    caracTmp = caracTmp >> flagRetenu;
                    // On enregistre le fait que cette ligne est pleine
                    matriceContenu[i]=30;
                    // On insére les bits dans la ligne (l'entier correspondant à la ligne)
                    caracTmp = caracTmp << (30 - matriceContenu[i]);
                    matrice[i] = matrice[i] ^ caracTmp;
                }
                else{
                    // Si tous les caractères ont été lus on bourre la dernière matrice qui correspond à la 28ème,
                    // On insère 32 puis des 0 à la suite de ce 32
                    if(nbCarac == 512 && nbPetiteMatrice == 27){
                        caracTmp = 32;
                    }
                    else{
                        if(nbCarac > 512 && nbPetiteMatrice == 27){
                            caracTmp = 0;
                        }
                        // Si ce n'est pas le cas on récupère le dernier caractère à lire
                        else{
                            caracTmp = message[compteur]; 
                            compteur++;
                        }
                    }
                    // On récupère et insére les bits dans la ligne (l'entier correspondant à la ligne)
                    nbCarac++;
                    matriceContenu[i]+=8;
                    caracTmp = caracTmp << (30 - matriceContenu[i]);
                    matrice[i] = matrice[i] ^ caracTmp;
                }
            }
        }
        // Ici on a une nouvelle matrice 25*25

        if(DEBUG) for (int j = 0; j < 5; j++) fprintf(stdout,"-----\n%d %d\n",matrice[j], matriceContenu[j]);
        
        // On effectue le calcul du TTH
        TTH(matrice,tabEmpreinte[w]);

        nbPetiteMatrice++;
    }
    return compteur;
}

//Fonction permettant de vérifier en remontant l'arbre de Merkle de deux feuilles à la racine l'intégrité d'une partie de l'arbre
int recursifInverse(int **tabArbre, int tailleArbre,int id){
    
    fprintf(stdout,"Vérification de id = [%d]\n",id);
    if (id == 0)
        return 0; // On est arrivé à la racine, l'intégrité est donc bonne
    else{
        int frere = -1, pere = -1 , flag = 0;

        if ((id % 2) == 0){ // si l'id est pair c'est que nous sommes sur un fils droit
            frere = id - 1;
            pere = id / 2 - 1;
        }else{ // si l'id est impair c'est que nous sommes sur un fils gauche
            frere =  id + 1;
            pere = id / 2;
        }

        int matrice[5] = {0};
        if (id%2 == 0) // si l'id est pair c'est que nous sommes sur un fils droit
        // c'est important que l'on insère les empreintes dans la matrice dans le bon ordre, le fils gauche en premier puis le fils droit. 
        // Or l'id peut être pair ou impair, il faut donc vérifier si l'id est pair ou impair pour savoir dans quel ordre insérer les empreintes dans la matrice
            for (int j = 0; j < 5; j++){
                int tmp = tabArbre[frere][j] << (24-j*6);
                matrice[0] = matrice[0] + tmp;
                tmp = tabArbre[id][j] << (24-j*6);
                matrice[1] = matrice[1] + tmp;
            }
        else
            for (int j = 0; j < 5; j++){
                int tmp = tabArbre[id][j] << (24-j*6);
                matrice[0] = matrice[0] + tmp;
                tmp = tabArbre[frere][j] << (24-j*6);
                matrice[1] = matrice[1] + tmp;
            }
        
        // on initialise l'empreinte à 0
        int empreinte[5] = {0};
        // on bourre la matrice
        matrice[2] = 32 << 24;
        TTH(matrice,empreinte);
        
        for (int i = 0; i < 5 && !flag; i++){ // on parcourt les 5 entiers de l'empreinte calculée et de l'empreinte originale
            if (tabArbre[pere][i] != empreinte[i]) // on vérifie si le père est bien égal à la somme des deux fils modulo 64, s'il ne l'est pas on met le flag à 1
                flag = 1;
        }

        if (flag) // si le flag est à 1, l'intégrité est mauvaise (une modification a été faite)
            return 1;
        else // sinon on continue la vérification
            return recursifInverse(tabArbre, tailleArbre,pere);

    }
}


// **** Contrôle d’intégrité global pour une partie ****
// Fonction qui vérifie si un bloc de data est bien intégré ou non
void controleIntegrite(char * message, int nbData, int idData, int ** tabArbre, int tailleArbre){
    
    fprintf(stdout,"\n====Début du contrôle d'intégrité pour le bloc n°%d====\n",idData);

    // Si le numéro du bloc n'est pas possible
    if (idData > nbData || idData < 0){
        if(DEBUG) fprintf(stdout,"Erreur : idData invalide\n");
        // return 1;
    }else{
        // On génère ici l'empreinte du bloc indiqué
        int **empreinte = (int**) malloc(sizeof(int*));
        empreinte[0] = (int*) calloc(5,sizeof(int));
        if (DataBlockHash(message,empreinte,512*idData,0) == 0) exit(1);

        // empreinte[0][0]++; // on modifie l'empreinte pour vérifier que l'intégrité est mauvaise
        // on compare l'empreinte calculée et l'empreinte de la feuille de l'arbre correspondant à l'empreinte du bloc data
        int flag = 0;
        for (int i = 0; i < 5 && !flag; i++){
            if (empreinte[0][i] != tabArbre[tailleArbre/2+idData][i])
                flag = 1;
        }

        // si les empreintes sont identiques, nous allons vérifier pour les parents
        if (!flag) // si les empreintes sont identiques (celle de la feuille et celle calculée)
            flag = recursifInverse(tabArbre, tailleArbre,tailleArbre/2+idData);


        if (flag) // si le flag est à 1, l'intégrité est mauvaise
            fprintf(stdout,"\nL'intégrité du bloc de données n°%d est mauvaise\n",idData);
        else // sinon l'intégrité est bonne
            fprintf(stdout,"\nL'intégrité du bloc de données n°%d est bonne\n",idData);

    }

    fprintf(stdout,"\n====Fin du contrôle d'intégrité====\n");
}



int main(int argc, char const *argv[]){
    // Fichier sur lequel on souhaite obtenir le hache
    FILE *fichier;
    // Taille du fichier en octet
    int taille;
    
    // On récupère le fichier à hacher
    if(argc < 2) {fprintf(stdout,"Veuillez saisir en argument le fichier à hacher !\n\n"); exit(1);}
    fichier = fopen(argv[1], "rb");
    if (fichier == NULL ){
        fprintf(stdout,"\nImpossible d'ouvrir le fichier '%s'\n\n", argv[1]);
        exit(1);
    }

    // On récupére la taille du fichier en octet
        fseek(fichier, 0, SEEK_END);
        taille = ftell(fichier);
        rewind(fichier);
        if(DEBUG) fprintf(stdout,"Taille du fichier : %d octets\n", taille);

        if (taille == 0){
            fprintf(stdout,"Le fichier est vide\n\n");
            exit(1);
        }
    // Fin de la récupération de la taille du fichier en octet
    


    // Création du tableau du message et ajout du padding s'il y a
        // Tableau du message 
        char *message = NULL;
        // nb cases de padding
        int padding = 0;
        
        if (taille % 512 != 0){
            if(DEBUG) fprintf(stdout,"Le fichier n'est pas un multiple de 512 octets\n");
            padding = 512 - taille % 512;
            taille = taille + padding;
            if(DEBUG) fprintf(stdout,"padding : %d et taille : %d\n",padding, taille);
        }
        // Allocation de la taille du message + padding si nécessaire
        message = (char*) malloc((taille) * sizeof(char));
    // Fin de la création du tableau du message

    
    // On remplit le tableau message
        char c;
        int cpt = 0;

        // On remplit le tableau message avec le contenu du fichier caractère par caractère
        fprintf(stdout,"Message :\n\n");
        while((c = fgetc(fichier)) != EOF){
            fprintf(stdout,"%c",c);
            message[cpt++] = c;
        }
        fprintf(stdout,"\n\n");

        // On ajoute le padding s'il y en a un
        if(padding != 0){
            message[cpt++] = 32;
            for (; cpt < taille; cpt++)
                message[cpt] = 0;
        }
        
        // On affiche le tableau du message 
        if(DEBUG){
            for (int j = 0; j < taille; j++)
                fprintf(stdout,"%d ",message[j]);
        }
    fclose(fichier);
    // Fin du remplissage du tableau message


    // Variable contenant le nombre de bloc de data de 512 octets généré par le message
        int nbBlockData = 0;
    // Variable temporaire permettant le bon déroulement de l'algorithme
        int compteur = 0;   // enregistre le dernier cararctère du message à lire

    // Tableau d'empreintes utilisés pour chaque bloc de 512 octets 
    int **tabEmreinte = (int**) malloc((taille/512)*sizeof(int*));
    for (int i = 0; i < taille/512; i++){
        *(tabEmreinte + i) = calloc(5,sizeof(int));
    }
    
    // On va maintenant traiter ici chaque caractère du fichier (message) :
    // Chaque itération de cette boucle = nouveau bloc de 512 octets
    for(int w = 0; nbBlockData < taille/512; w++){
        if(DEBUG) fprintf(stdout,"\n-------------NEW BLOC DATA-%d-----------\n",w);
        compteur = DataBlockHash(message,tabEmreinte,compteur,w);
        nbBlockData++;
    }

    //affichage du nombre de bloc de données
    fprintf(stdout,"\nIl y a eu %d bloc de données générés\n\n",nbBlockData);

    // // On affiche les empreintes
    // for (int i = 0; i < nbBlockData; i++){
    //     for (int j = 0; j < 5; j++){
    //         fprintf(stdout,"%d ",tabEmreinte[i][j]);
    //     }
    //     fprintf(stdout,"\n");
    // }

    // ************************** Début de la partie 2 : Arbre de Merkle **************************
    
    // On va ici calculer le nombre de noeud à ajouter pour que le nombre de noeud soit une puissance de 2 (pour la construction de l'arbre)
        double puissance = log2(nbBlockData); // On récupère la puissance de 2 la plus proche
        int ajout = 0; // Nombre de noeud à ajouter
        int nbFeuille = nbBlockData; // Nombre de feuille de l'arbre
        if(DEBUG) fprintf(stdout,"puissance : %f\n", puissance);
        if (puissance != (int)puissance){ // Si la puissance n'est pas un entier alors le nombre de noeud n'est pas une puissance de 2
            if(DEBUG) fprintf(stdout,"Le nombre de bloc n'est pas une puissance de 2\n");
            ajout = pow(2, (int)puissance+1) - nbBlockData; // On calcule le nombre de noeud à ajouter
            if(DEBUG) fprintf(stdout,"noeud à ajouter : %d\n", ajout);
            tabEmreinte = realloc(tabEmreinte, (nbBlockData+ajout)*sizeof(int*)); // On alloue de la mémoire pour les noeuds à ajouter
            for (int i = nbBlockData; i < nbBlockData+ajout; i++){ // On initialise les noeuds à ajouter
                tabEmreinte[i] = calloc(5,sizeof(int));
            }
            nbFeuille += ajout; // On met à jour le nombre de feuille
            fprintf(stdout,"L'arbre de Merkel est composé de %d feuilles\n", nbFeuille);
        }
    
        // Pour créer l'arbre de merkle on va utiliser un tableau de pointeur sur int
        // On va créer un tableau de taille 2*nbFeuille-1
        // On va ensuite remplir le tableau avec les empreintes des blocs de données
        // On alloue la mémoire pour le tableau
        int **tabArbre = calloc(2*nbFeuille-1, sizeof(int*)); // On alloue la mémoire pour le tableau, nous avons une empreinte par cellule

        // On initialise le tableau pour les noeuds de l'arbre non feuilles
        for (int i = 0; i < nbFeuille-1; i++){
            tabArbre[i] = calloc(5,sizeof(int)); 
        }
        // À une position i du tableau, les fils gauche et droit sont à 2*i+1 et 2*i+2
        // On va donc remplir le tableau en partant de la fin
        // On va commencer par remplir les feuilles
        for (int i = 0; i < nbFeuille; i++){
            tabArbre[nbFeuille-1+i] = tabEmreinte[i];
        }

        fprintf(stdout,"\nÉtat de la structure de l'arbre de Merkle après remplissage des feuilles :\n");
        fprintf(stdout,"\n(Numéro du noeud)\t(hache associé)\n");
        // On affiche le tableau
        for (int i = 0; i < 2*nbFeuille-1; i++){
            fprintf(stdout,"bloc [%d]\t\t: ",i);
            for (int j = 0; j < 5; j++){
                fprintf(stdout,"%d ",tabArbre[i][j]);
            }
            fprintf(stdout,"\n");
        }

        // On va maintenant remplir les noeuds non feuilles
        for (int i = nbFeuille-2; i >= 0; i--){
            // On ajoute le hache des fils gauche et droit auquel on fait un modulo 64
            // C'est ici qu'à lieu la concaténation de deux haches de deux frères
            int matrice[5] = {0};
            for (int j = 0; j < 5; j++){
                int tmp = tabArbre[2*i+1][j] << (24-j*6);
                matrice[0] = matrice[0] + tmp;
                tmp = tabArbre[2*i+2][j] << (24-j*6);
                matrice[1] = matrice[1] + tmp;
            }
            // On bourre la matrice, comme indiqué dans le readme.txt
            matrice[2] = 32 << 24;
            TTH(matrice,tabArbre[i]);
        }
        
        fprintf(stdout,"\nÉtat de la structure de l'arbre de Merkle après voir ajouté les noeuds non feuilles dans l'arbre de Merkle :\n");
        fprintf(stdout,"\n(Numéro du bloc)\t(hache associé)\n");
        // On affiche le tableau
        for (int i = 0; i < 2*nbFeuille-1; i++){
            fprintf(stdout,"bloc [%d]\t\t: ",i);
            for (int j = 0; j < 5; j++){
                fprintf(stdout,"%d ",tabArbre[i][j]);
            }
            fprintf(stdout,"\n");
        }

        fprintf(stdout,"\n\nParcours de l'arbre en profondeur gauche :\n");
        fprintf(stdout,"\n(Numéro du bloc)\t(hache associé)\n");
        parcoursArbre(tabArbre, 0, 2*nbFeuille-1);

    // Début de la procédure de vérification du contrôle d’intégrité
    int choix = 0;
    fprintf(stdout,"\nVeuillez choisir un bloc de données à vérifier (entre 0 et %d) : ",nbBlockData);
    scanf("%d",&choix);
    while (choix < 0 || choix > nbBlockData-1){
        fprintf(stdout,"Veuillez choisir un bloc de données à vérifier (entre 0 et %d) : ",nbBlockData);
        scanf("%d",&choix);
    }

    // On va maintenant vérifier l'intégrité du bloc de data numéro "choix"
    controleIntegrite(message,nbBlockData,choix,tabArbre,nbFeuille*2-1);

    // Libération de la mémoire dans le tas
    for (int i = 0; i < nbBlockData; i++){
        free(tabEmreinte[i]);
    }
    free(tabEmreinte);
    free(message);

    // On termine le programme
    fprintf(stdout,"\nFin du programme\n");
    return EXIT_SUCCESS;
}