#include <iostream>
#include <opencv2/opencv.hpp>
#include <math.h>


// Données transmises aux callbacks ----------------------------------------

class My {
  public:
    cv::Mat img_res1, img_res2, img_src;
    int flag1 = 0; // Pour l'actualisation de l'affichage (Première actualisation en dehors du for)
	int flag2 = 0; // Pour gérer les clics glissés à la souris 
    int seuil = 5;
    int seuil_max = 20;
	int x_clic; // Position (x,y) du centre de la loupe
	int y_clic;
};

// Dessin du rectangle (pas_x x pas_y) selon le point  x0, y0 --------------

void dessine_carre(My &my, int x0, int y0, int * point, int pas_x, int pas_y) 
{	
    int y1 = y0;
    int x1 = x0;
    int y2 = y0 + pas_y;
    int x2 = x0 + pas_x;

	for (int y = y1; y < y2; y++) // Accès au point et modifications
	for (int x = x1; x < x2; x++) {
	    my.img_res2.at<cv::Vec3b>(y,x)[0] = point[0]; 
	    my.img_res2.at<cv::Vec3b>(y,x)[1] = point[1];
	    my.img_res2.at<cv::Vec3b>(y,x)[2] = point[2];
	}
}

// Dessine le carré correspondant à la zone de la loupe ---------------------

void dessine_loupe(My &my) 
{
    if (my.img_res1.type() != CV_8UC3) {
        std::cout << __func__ << ": format non géré :" << my.img_res1.type() << std::endl;
        return;
    }

    int y1 = my.y_clic-my.seuil ; // Sommets de la loupe
    int x1 = my.x_clic-my.seuil ;
    int y2 = my.y_clic+my.seuil ; 
    int x2 = my.x_clic+my.seuil ; 

	if (x1 < 0) {x1 = 0; x2 = 2*my.seuil;} // Conditions aux bords
	if (y1 < 0) {y1 = 0; y2 = 2*my.seuil;}
	if (x2 >= my.img_res1.cols) {x2 = my.img_res1.cols -1 ; x1 = x2 - 2*my.seuil ;}
	if (y2 >= my.img_res1.rows) {y2 = my.img_res1.rows -1 ; y1 = y2 - 2*my.seuil ;}

	my.x_clic = x2 - my.seuil; // Actualisation du centre de la loupe
	my.y_clic = y2 - my.seuil;

    for (int y = y1+1; y < y2; y++) // Dessin du carré blanc
	{
        my.img_res1.at<cv::Vec3b>(y,x1)[0] = 255;
        my.img_res1.at<cv::Vec3b>(y,x1)[1] = 255;
        my.img_res1.at<cv::Vec3b>(y,x1)[2] = 255;
        my.img_res1.at<cv::Vec3b>(y,x2)[0] = 255;
        my.img_res1.at<cv::Vec3b>(y,x2)[1] = 255;
        my.img_res1.at<cv::Vec3b>(y,x2)[2] = 255;
    }    
	for (int x = x1; x <= x2; x++) 
	{
        my.img_res1.at<cv::Vec3b>(y1,x)[0] = 255;
        my.img_res1.at<cv::Vec3b>(y1,x)[1] = 255;
        my.img_res1.at<cv::Vec3b>(y1,x)[2] = 255;
        my.img_res1.at<cv::Vec3b>(y2,x)[0] = 255;
        my.img_res1.at<cv::Vec3b>(y2,x)[1] = 255;
        my.img_res1.at<cv::Vec3b>(y2,x)[2] = 255;
    }
    for (int x = x1+1; x < x2; x++) // Dessin du carré rouge dans le carré blanc
	{
        my.img_res1.at<cv::Vec3b>(y1+1,x)[0] = 0;
        my.img_res1.at<cv::Vec3b>(y1+1,x)[1] = 0;
        my.img_res1.at<cv::Vec3b>(y1+1,x)[2] = 255;
        my.img_res1.at<cv::Vec3b>(y2-1,x)[0] = 0;
        my.img_res1.at<cv::Vec3b>(y2-1,x)[1] = 0;
        my.img_res1.at<cv::Vec3b>(y2-1,x)[2] = 255;
    }
    for (int y = y1+1; y < y2; y++)
	{
        my.img_res1.at<cv::Vec3b>(y,x1+1)[0] = 0;
        my.img_res1.at<cv::Vec3b>(y,x1+1)[1] = 0;
        my.img_res1.at<cv::Vec3b>(y,x1+1)[2] = 255;
        my.img_res1.at<cv::Vec3b>(y,x2-1)[0] = 0;
        my.img_res1.at<cv::Vec3b>(y,x2-1)[1] = 0;
        my.img_res1.at<cv::Vec3b>(y,x2-1)[2] = 255;
     }  
}

// Calcul des images a afficher. On ne modifie jamais img_src ici -------------

void calculer_images_resultat (My &my)
{
	my.img_res2 = cv::Mat(200, 200, CV_8UC3, cv::Scalar(0,0,0)); // Réinitialisation image zoomée
	
	// Définitions de variables pour ajuster les pixels de certains points, et avoir une taille d'image zoomé constante (Points au centre de l'image zoomé agrandi d'un pixel)
	int pas = my.img_res2.rows / (2*my.seuil+1); // Longueur du point zoomé en x ou y 
	int borne_max = pas * (2 * my.seuil + 1); // Bornes avant ajustement de la loupe zoomé
	int rest = my.img_res2.rows - borne_max; // Les pixels restants non modifiés sur res2
	int debut = (borne_max/(2*pas) - rest/2)*pas; // Début des points zoomés d'un pixel en plus
	int fin = debut + pas * (rest-1) + rest; // Fin des points zoomés d'un pixel en plus
	int flag = 0; // Pour les ajutements en y
	int p[3]; // Point en cours de zoom,
    int x_p = my.x_clic - my.seuil; // Position (x,y) du point en cours de zoom
	int y_p = my.y_clic - my.seuil;  
	int x = 0;  // Position (x,y) du début du carré du point à zoomer
	int y = 0;

    while (y < my.img_res2.rows) // Parcours par saut de seuil 
	{
    	while (x < my.img_res2.rows) 
		{
			p[0] = my.img_src.at<cv::Vec3b>(y_p,x_p)[0];
    		p[1] = my.img_src.at<cv::Vec3b>(y_p,x_p)[1];
    		p[2] = my.img_src.at<cv::Vec3b>(y_p,x_p)[2];
			
			if (x >= debut && x <= fin) { // Gestion des cas pour combler tous les pixels de res2
				if (y >= debut && y <= fin) {
					dessine_carre (my,x,y,p,pas+1,pas+1);
					x++ ;
					flag = 1;
				}
				else {
					dessine_carre (my,x,y,p,pas+1,pas);
					x++ ;
				}
			}
			else {
				if (y >= debut && y <= fin) {
					dessine_carre (my,x,y,p,pas,pas+1);
					flag = 1;
				}
				else {
					dessine_carre (my,x,y,p,pas,pas);
				}
			}
			x += pas; // Actualisation du pixel de debut du zoom du nouveau point	
			x_p += 1; // Actualisation du point en cours de zoom
		}
		y += pas; // Actualisation du pixel de debut du zoom du nouveau point
 		x = 0;
		if(flag) {y++; flag = 0;} 
		y_p += 1; // Actualisation du point en cours de zoom
		x_p = my.x_clic - my.seuil;
	}
}

// Callback du slider ---------------------------------------------------------

void onTrackbarSlide (int pos, void *data)
{
    My *my = (My*) data;
	my->seuil = (!my->seuil)? 1 : my->seuil; // Gestion du cas my->seuil = 0 
    my->flag1 = 1;  // recalculer et réafficher
	my->img_res1 = my->img_src.clone(); // Pour l'actualisation de la loupe
}

// Callback pour la souris ----------------------------------------------------

void onMouseEvent (int event, int x, int y, int flags, void *data)
{
    My *my = (My*) data;

    switch (event) {
        case cv::EVENT_LBUTTONDOWN :
			my->img_res1 = my->img_src.clone();
            my->flag1 = 1; 
			my->flag2 = 1; // Indique que la souris passe en mode cliqué
			my->x_clic = x; 
			my->y_clic = y;
            break;
        case cv::EVENT_MOUSEMOVE :
			if(my->flag2) {
				my->img_res1 = my->img_src.clone();
            	my->flag1 = 1;
				my->x_clic = x; 
				my->y_clic = y;
			}
            std::cout << "mouse move " << x << "," << y << std::endl;
            break;
        case cv::EVENT_LBUTTONUP :
            std::cout << "mouse button up" << std::endl;
            my->flag2 = 0;
            break;
    }
}

// Callback "maison" pour le clavier -------------------------------------------

int onKeyPressEvent (int key, void *data)
{
    My *my = (My*) data;

    if (key < 0) return 0;        // aucune touche pressée
    key &= 255;                   // pour comparer avec un char
    if (key == 27) return -1;     // ESC pour quitter
	if (key != 255) std::cout << "Touche '" << char(key) << "'" << std::endl;
    return 1;
}

// Main ------------------------------------------------------------------------

int main (int argc, char**argv)
{
   if (argc-1 != 2) {
        std::cout << "Usage: " << argv[0] << " in1 out2" << std::endl;
        return 1;
    }

    My my;

    // Lecture image

    my.img_src = cv::imread (argv[1], cv::IMREAD_COLOR);
    if (my.img_src.empty()) {
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

	// Intialisations

	my.img_res1 = my.img_src.clone();
	my.img_res2 = cv::Mat(200, 200, CV_8UC3, cv::Scalar(0,0,0));
	my.x_clic = my.img_src.cols/2; // On place le zoom par défaut au milieu
	my.y_clic = my.img_src.rows/2;

    // Création fenêtres

    cv::namedWindow ("Source", cv::WINDOW_NORMAL);
    cv::createTrackbar ("Seuil", "Source", &my.seuil, my.seuil_max, onTrackbarSlide, &my);
    cv::setMouseCallback ("Source", onMouseEvent, &my);
    cv::namedWindow ("Zoom", cv::WINDOW_AUTOSIZE);

    std::cout << "Pressez : ESC pour enregistrer" << std::endl;

    cv::imshow ("Source", my.img_res1);
    cv::imshow ("Zoom", my.img_res2);

    // Boucle d'évènements

    for (;;) {

        // On recalcule img_res1/res2 et on réaffiche si le flag est a 1

        if (my.flag1) {
            std::cout << "Calcul images puis affichage" << std::endl;
			dessine_loupe(my);	
            calculer_images_resultat (my);
            cv::imshow ("Source", my.img_res1);
            cv::imshow ("Zoom", my.img_res2);
            my.flag1 = 0;
        }

        // Attente du prochain évènement sur toutes les fenêtres

        int key = cv::waitKey (15);

        // Gestion des évènements clavier avec une callback "maison" que l'on appelle nous-même. Les Callbacks souris et slider sont directement appelés par waitKey lors de l'attente.

        if (onKeyPressEvent (key, &my) < 0) break;
    }

    // Enregistrement résultat

    if (! cv::imwrite (argv[2], my.img_res2))
         std::cout << "Erreur d'enregistrement" << std::endl;
    else std::cout << "Enregistrement effectué" << std::endl;

    return 0;
}


