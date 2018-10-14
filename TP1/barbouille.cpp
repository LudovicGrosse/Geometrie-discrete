#include <iostream>
#include <opencv2/opencv.hpp>
#include <math.h>

struct My { // Structure pour les données de la trackbar
    cv::Mat img; 
    int flag; // 1 si la souris est appuyée, 0 sinon
	int ray; // Rayon du cercle
	int r; // Couleur rouge
	int g; // Couleur verte
	int b; // Couleur bleue
};

void dessine(int x0, int y0, My * data) // Dessin un cercle par clic souris
{
    if (data->img.type() != CV_8UC3) { // Si le format n'est pas géré, erreur
        std::cout << __func__ << ": format non géré :" << data->img.type() << std::endl;
        return;
    }

    int y1 = y0-data->ray ; if (y1 < 0) y1 = 0; // Si on est à un bord du début
    int x1 = x0-data->ray ; if (x1 < 0) x1 = 0;
    int y2 = y0+data->ray ; if (y2 > data->img.rows) y2 = data->img.rows; // Si on est à un bord de la fin
    int x2 = x0+data->ray ; if (x2 > data->img.cols) x2 = data->img.cols; 

    for (int y = y1; y < y2; y++) // Accès au pixels et modifications
    for (int x = x1; x < x2; x++) {
		if ((pow(x-x0,2) + pow(y-y0,2)) <= pow(data->ray,2)){
		    data->img.at<cv::Vec3b>(y,x)[0] = data->b; // On utilise les couleurs définies par les datas r, g, b
		    data->img.at<cv::Vec3b>(y,x)[1] = data->g;
		    data->img.at<cv::Vec3b>(y,x)[2] = data->r;
		}
    }
}

void onTrackbarSlide (int pos, void *data) // Callback de la trackbar
{
    My *my = (My*) data;
}

void onMouseEvent (int event, int x, int y, int flags, void *data) // Callback de la souris
{
    My *my = (My*) data;

    switch (event) {
        case cv::EVENT_LBUTTONDOWN : // Si clic bas, on créé le cercle
            std::cout << "mouse button down" << std::endl;
            dessine(x, y, my);
            cv::imshow ("Example1", my->img);
			my->flag = 1; 
            break;
        case cv::EVENT_MOUSEMOVE : // Déplacement souris
            std::cout << "mouse move " << x << "," << y << std::endl;
            if (my->flag) { // Si le clic gauche est toujours enclenché
				dessine(x, y, my);
				cv::imshow ("Example1", my->img);
			}
            break;
        case cv::EVENT_LBUTTONUP : // Clic haut 
            std::cout << "mouse button up" << std::endl;
			my->flag = 0;
            break; 
			
    }
}

int main (int argc, char**argv)
{
	// Si erreur d'arguments -----------------------------------------

    if (argc-1 != 2) { // Si il manque des arguments
        std::cout << "Usage: " << argv[0] << " in1 out2" << std::endl;
        return 1;
    }

	// Initalisation structure ----------------------------------------

    My my; 
    my.ray = 50; // Rayon de base
	my.r = 127; // Couleurs de base
	my.g = 127;
	my.b = 127;
    my.flag = 0; // Première actualisation

	// Si erreur de lecture ------------------------------------------

    my.img = cv::imread (argv[1], cv::IMREAD_COLOR); // Chargement image
    if (my.img.empty()) { // Si l'image est vide
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

	// Initialisation fenêtre -----------------------------------------

    cv::namedWindow ("Example1", cv::WINDOW_NORMAL); // Chargement fenetre
    cv::createTrackbar ("Rayon", "Example1", &my.ray, 100, onTrackbarSlide, &my); // Trackbar 1
    cv::createTrackbar ("R", "Example1", &my.r, 255, onTrackbarSlide, &my); // Trackbar 2
    cv::createTrackbar ("G", "Example1", &my.g, 255, onTrackbarSlide, &my); // Trackbar 3
    cv::createTrackbar ("B", "Example1", &my.b, 255, onTrackbarSlide, &my); // Trackbar 4
    cv::setMouseCallback ("Example1", onMouseEvent, &my); // Evenements souris
    std::cout << "Pressez ESC pour quitter, espace pour recalculer" << std::endl;
    
	// Affichage interactif -------------------------------------------

	cv::imshow ("Example1", my.img);

    for (;;) {
        int key = cv::waitKey (15);   // timeout pour détecter changements flags
        if (key < 0) continue;        // aucune touche pressée
        key &= 255;                   // pour comparer avec un char
        if (key == 27) break;
		std::cout << "Touche '" << char(key) << "'" << std::endl;
    }

	// Enregistrement -------------------------------------------------

    if (! cv::imwrite (argv[2], my.img))
         std::cout << "Erreur d'enregistrement" << std::endl;
    else std::cout << "Enregistrement effectué" << std::endl;

    return 0;
}



