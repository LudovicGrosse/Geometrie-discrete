    #include <iostream>
    #include <opencv2/opencv.hpp>
	#include <stdio.h> 
	using namespace std;

struct My { // Structure pour les données de la trackbar
    cv::Mat img, res, inv; // Image source, résultat et inverse
    int flag;
    int seuil;
};

void dessine_carre (int x, int y, My * data) // Dessin du carré par clic souris où (x,y) est la position en haut à gauche du carré
{
    if (data->img.type() != CV_8UC3) { // Si le format n'est pas géré, erreur
        std::cout << __func__ << ": format non géré :" << data->img.type() << std::endl;
        return;
    }

    int y1 = y; 
    int x1 = x;
    int y2 = y + data->seuil ; if (y2 > data->img.rows) y2 = data->img.rows; // Si on est sur un bord (A droite ou en bas)
    int x2 = x + data->seuil ; if (x2 > data->img.cols) x2 = data->img.cols; 

	if (data->flag != 2) { // Dessins de carrés blancs
		for (int y = y1; y < y2; y++) // Accès au pixels et modifications
		for (int x = x1; x < x2; x++) {
		    data->res.at<cv::Vec3b>(y,x)[0] = 255; 
		    data->res.at<cv::Vec3b>(y,x)[1] = 255;
		    data->res.at<cv::Vec3b>(y,x)[2] = 255;
		}
	}
	else { // Dessins avec l'image donnée en arguments
		for (int y = y1; y < y2; y++) // Accès au pixels et modifications
		for (int x = x1; x < x2; x++) {
		    data->res.at<cv::Vec3b>(y,x)[0] = data->inv.at<cv::Vec3b>(y,x)[0]; 
		    data->res.at<cv::Vec3b>(y,x)[1] = data->inv.at<cv::Vec3b>(y,x)[1];
		    data->res.at<cv::Vec3b>(y,x)[2] = data->inv.at<cv::Vec3b>(y,x)[2];
		}
	}
}

void onTrackbarSlide (int pos, void * data) // Callback de la trackbar
{
    My *my = (My *) data;
	my->res = my->img.clone();	
	my->seuil = (!my->seuil)? 1 : my->seuil; // Gestion du cas my->seuil = 0
	int aux = 0;

    for (int y = 0; y < my->img.rows; y += my->seuil) // Parcours par saut de seuil (Morceaux de carré à la fin)
	{
    	for (int x = aux; x < my->img.cols; x += my->seuil*2) 
		{
			dessine_carre (x, y, my);
		}
		aux = my->seuil - aux ; // Inversion à chaque ligne 
	}
	cv::imshow ("Example1", my->res);
}

void inverse(cv::Mat &img) 
{
    if (img.type() != CV_8UC3) { // Si le format est utilisable dans notre cas
        std::cout << __func__ << ": format non géré :" << img.type() << std::endl;
        return;
    }

    for (int y = 0; y < img.rows; y++) // Parcours les lignes et des colonnes
	{
    	for (int x = 0; x < img.cols; x++) 
		{
			img.at<cv::Vec3b>(y,x)[0] = 255 - img.at<cv::Vec3b>(y,x)[0]; // Inversion des pixels 
        	img.at<cv::Vec3b>(y,x)[1] = 255 - img.at<cv::Vec3b>(y,x)[1];
        	img.at<cv::Vec3b>(y,x)[2] = 255 - img.at<cv::Vec3b>(y,x)[2];
		}	
	}
}


int main (int argc, char**argv)
{
	// Si erreur d'arguments -----------------------------------------

    if (argc-1 != 2) {
        std::cout << "Usage: " << argv[0] << " image1 image2" << std::endl;
        return 1;
    }

	// Chargement des 3 images ----------------------------------------

	cv::Mat src = cv::imread (argv[1], cv::IMREAD_COLOR); // Image de base
	cv::Mat inv = src.clone(); // Image inverse
	inverse(inv);
	cv::Mat blanc = cv::Mat(src.rows, src.cols, CV_8UC3, cv::Scalar(0,0,0)); // Image blanche


	// Initalisation structure ----------------------------------------

	My my;
	my.seuil = 50; // Carré de base
	my.flag = 0; // Première actualisation
    my.img = blanc.clone(); 
	my.inv = inv.clone();

	// Si erreur de lecture ------------------------------------------

    if (my.img.empty()) {
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

	// Initialisation fenêtre -----------------------------------------

    cv::namedWindow ("Example1", cv::WINDOW_AUTOSIZE);
	cv::createTrackbar ("Seuil", "Example1", &my.seuil, 100, onTrackbarSlide, &my); // Trackbar


	// Affichage interactif -------------------------------------------
    
    std::cout << "Pressez une touche dans l'image pour quitter." << std::endl;
	onTrackbarSlide (my.seuil, &my); // Image de base

	for (;;) {
		int key = cv::waitKey (150); // Timeout pour détecter changements flags
		if (key < 0) continue; // Aucune touche pressée
		key &= 255;  // Pour comparer avec un char (Comparaison bit à bit)
		if (key == 27) break;

		if (key == ' '){
			std::cout << "Touche espace" << std::endl;
		    my.flag = (my.flag+1)%3; // Le flag varie entre 0 et 2
			if (my.flag == 0) {my.img = blanc.clone(); onTrackbarSlide (my.seuil, &my);} // L'image à traiter est l'image blanche
			if (my.flag == 1) {my.img = src.clone(); onTrackbarSlide (my.seuil, &my);} // L'image à traiter est l'image source
			if (my.flag == 2) {onTrackbarSlide (my.seuil, &my);} // L'image à traiter est encore l'image source mais avec un damier de l'image inverse (traité dans la callback)	
		}
		else
		{
		    std::cout << "Touche '" << char(key) << "'" << std::endl;
		}
	}

	// Enregistrement -------------------------------------------------

	if (! cv::imwrite (argv[2], my.res))
	     std::cout << "Erreur d'enregistrement" << std::endl;
	else std::cout << "Enregistrement effectué" << std::endl;

    return 0;
}
