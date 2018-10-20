#include <iostream>
#include <cstring>
#include <opencv2/opencv.hpp>

//--------------------------------- L O U P E ---------------------------------

class Loupe {
  public:
    int zoom = 5;
    int zoom_max = 20;
    int zoom_x0 = 0;
    int zoom_y0 = 0;
    int zoom_x1 = 100;
    int zoom_y1 = 100;

    void reborner (cv::Mat &res1, cv::Mat &res2)
    {
        int bon_zoom = zoom >= 1 ? zoom : 1;

        int h = res2.rows / bon_zoom;
        int w = res2.cols / bon_zoom;

        if (zoom_x0 < 0) zoom_x0 = 0;
        zoom_x1 = zoom_x0 + w;
        if (zoom_x1 > res1.cols) {
            zoom_x1 = res1.cols;
            zoom_x0 = zoom_x1 - w;
            if (zoom_x0 < 0) zoom_x0 = 0;
        }

        if (zoom_y0 < 0) zoom_y0 = 0;
        zoom_y1 = zoom_y0 + h;
        if (zoom_y1 > res1.rows) {
            zoom_y1 = res1.rows;
            zoom_y0 = zoom_y1 - h;
            if (zoom_y0 < 0) zoom_y0 = 0;
        }
    }

    void deplacer (cv::Mat &res1, cv::Mat &res2, int dx, int dy)
    {
        zoom_x0 += dx; zoom_y0 += dy; 
        zoom_x1 += dx; zoom_y1 += dy; 
        reborner (res1, res2);
    }

    void dessiner_rect (cv::Mat &src, cv::Mat &dest)
    {
        dest = src.clone();
        if (zoom == 0) return;
        cv::Point p0 = cv::Point(zoom_x0, zoom_y0),
                  p1 = cv::Point(zoom_x1, zoom_y1);
        cv::rectangle(dest, p0, p1, cv::Scalar (255, 255, 255), 3, 4);
        cv::rectangle(dest, p0, p1, cv::Scalar (  0,   0, 255), 1, 4);
    }

    void dessiner_portion (cv::Mat &src, cv::Mat &dest)
    {
        if (!(src.type() == CV_8UC3 )) {
            std::cout << __func__ << ": format non géré :" << src.type() 
                      << std::endl;
            return;
        }

        int bon_zoom = zoom >= 1 ? zoom : 1;

        for (int y = 0; y < dest.rows; y++)
        for (int x = 0; x < dest.cols; x++)
        {
            int x0 = zoom_x0 + x / bon_zoom;
            int y0 = zoom_y0 + y / bon_zoom;

            if (x0 < 0 || x0 >= src.cols || y0 < 0 || y0 >= src.rows) {
                dest.at<cv::Vec3b>(y,x)[0] = 64;
                dest.at<cv::Vec3b>(y,x)[1] = 64;
                dest.at<cv::Vec3b>(y,x)[2] = 64;
                continue;
            }
            dest.at<cv::Vec3b>(y,x)[0] = src.at<cv::Vec3b>(y0,x0)[0];
            dest.at<cv::Vec3b>(y,x)[1] = src.at<cv::Vec3b>(y0,x0)[1];
            dest.at<cv::Vec3b>(y,x)[2] = src.at<cv::Vec3b>(y0,x0)[2];
        }
    }
};


//----------------------- C O U L E U R S   V G A -----------------------------

void representer_en_couleurs_vga (cv::Mat img_niv, cv::Mat img_coul)
{
    unsigned char couls[256][3] = {  // R, G, B
        {   0,   0,   0 },   //  0               : black
        {  20,  20, 190 },   //  1, 15, 29, ...  : blue
        {  30, 200,  30 },   //  2, 16, ...      : green
        {  30, 200, 200 },   //  3, 17, ...      : cyan
        { 200,  30,  30 },   //  4, 18, ...      : red
        { 200,  30, 200 },   //  5, 19, ...      : magenta
        { 200, 130,  50 },   //  6, 20, ...      : brown
        { 200, 200, 200 },   //  7, 21, ...      : light gray
        { 110, 110, 140 },   //  8, 22, ...      : dark gray
        {  84, 130, 252 },   //  9, 23, ...      : light blue
        {  84, 252,  84 },   // 10, 24, ...      : light green
        {  84, 252, 252 },   // 11, 25, ...      : light cyan
        { 252,  84,  84 },   // 12, 26, ...      : light red
        { 252,  84, 252 },   // 13, 27, ...      : light magenta
        { 252, 252,  84 },   // 14, 28, ...      : yellow
        { 252, 252, 252 },   // 255              : white
    };
    couls[255][0] = couls[15][0];
    couls[255][1] = couls[15][1];
    couls[255][2] = couls[15][2];

    for (int i = 15; i < 255; i++) {
        int j = 1 + (i-1)%14;
        couls[i][0] = couls[j][0];
        couls[i][1] = couls[j][1];
        couls[i][2] = couls[j][2];
    }

    for (int y = 0; y < img_niv.rows; y++)
    for (int x = 0; x < img_niv.cols; x++)
    {
        int g = img_niv.at<uchar>(y,x);
        // Attention img_coul est en B, G, R -> inverser les canaux
        img_coul.at<cv::Vec3b>(y,x)[0] = couls[g][2];
        img_coul.at<cv::Vec3b>(y,x)[1] = couls[g][1];
        img_coul.at<cv::Vec3b>(y,x)[2] = couls[g][0];
    }
}


//----------------------------------- M Y -------------------------------------

class My {
  public:
    cv::Mat img_src, img_res1, img_res2, img_niv, img_coul;
    Loupe loupe;
    int seuil = 127;
    int clic_x = 0;
    int clic_y = 0;
    int clic_n = 0;
	int connexite = 1; // 1 pour 4, 2 pour 8

    enum Recalc { R_RIEN, R_LOUPE, R_TRANSFOS, R_SEUIL };
    Recalc recalc = R_SEUIL;

    void reset_recalc ()             { recalc = R_RIEN; }
    void set_recalc   (Recalc level) { if (level > recalc) recalc = level; }
    int  need_recalc  (Recalc level) { return level <= recalc; }

    // Rajoutez ici des codes A_TRANSx pour le calcul et l'affichage
    enum Affi { A_ORIG, A_SEUIL, A_TRANS1, A_TRANS2, A_TRANS3 };
    Affi affi = A_ORIG;
};


//----------------------- T R A N S F O R M A T I O N S -----------------------

void inverser_couleurs (cv::Mat img)
{
    if (img.type() != CV_8UC3) {
        std::cout << __func__ << ": format non géré :" << img.type() << std::endl;
        return;
    }

    for (int y = 0; y < img.rows; y++)
    for (int x = 0; x < img.cols; x++)
    {
        img.at<cv::Vec3b>(y,x)[0] = 255 - img.at<cv::Vec3b>(y,x)[0];
        img.at<cv::Vec3b>(y,x)[1] = 255 - img.at<cv::Vec3b>(y,x)[1];
        img.at<cv::Vec3b>(y,x)[2] = 255 - img.at<cv::Vec3b>(y,x)[2];
    }
}


// Placez ici vos fonctions de transformations à la place de ces exemples

// Transformation 1 --------------------------

bool est_un_contour(cv::Mat img_niv, int x, int y, int pix, int coul, int connexite) // Pix est un point blanc
{
	if (x == 0 || x == img_niv.cols - 1 || y == 0 || y == img_niv.rows - 1) return true; // Si au bord
	int voisin, n;
	int v_x[8] = {-1,1,0,0,-1,-1,1,1};
	int v_y[8] = {0,0,-1,1,-1,1,-1,1,}; 
	
	if (connexite == 0) n = 4;
	else n = 8;
	
	for (int i = 0; i < n; i++){
		voisin = img_niv.at<uchar>(y + v_y[i],x + v_x[i]);
		if (voisin == coul) return true;
	}
	return false;
}

void effectuer_pelage_contours (cv::Mat img_niv , int connexite)
{ 
 	if (img_niv.type() != CV_8UC1) {
        std::cout << __func__ << ": format non géré :" << img_niv.type() << std::endl;
        return;
    }
	int coul, coul_anc, pix;
	int i = 0;
	bool blanc  = true; // Pour la condition d'arrêt (Plus de blanc sur la photo)

	while (blanc) {
		blanc = false;
		i ++ ;
	    coul = (i) % 255 ;
		coul_anc = (i - 1) % 255 ;

		for (int y = 0; y < img_niv.rows; y++)
		for (int x = 0; x < img_niv.cols; x++)
		{
			pix = img_niv.at<uchar>(y,x);
			if (pix == 255 && est_un_contour(img_niv, x, y, pix, coul_anc, connexite)) 
			{
				img_niv.at<uchar>(y,x) = coul;
				blanc = true;
			}
		}
	}
}

// Transformation 2 --------------------------

void detecter_maximums_locaux (cv::Mat img_niv, int connexite)
{
    if (img_niv.type() != CV_8UC1) {
        std::cout << __func__ << ": format non géré :" << img_niv.type() << std::endl;
        return;
    }
	cv::Mat img_pelee = img_niv.clone(); // On effectue le pelage sur une copie de l'image
	effectuer_pelage_contours (img_pelee, connexite);

	int voisin, x_i, y_i, pix, max;
	int v_x[8] = {-1,1,0,0,-1,-1,1,1};
	int v_y[8] = {0,0,-1,1,-1,1,-1,1,}; 
	int n = connexite ? 8 : 4;
	
    for (int y = 0; y < img_niv.rows; y++)
    for (int x = 0; x < img_niv.cols; x++)
    {
		pix = img_pelee.at<uchar>(y,x);
		max = pix; ;
		if (pix != 0)
		{
			for (int i = 0; i < n; i++)
			{
				x_i = x + v_x[i];
				y_i = y + v_y[i];
				if (x_i >= 0 && y_i >= 0 && x_i < img_niv.cols && y_i < img_niv.rows) 
				{
					voisin = img_pelee.at<uchar>(y_i,x_i);
					if (voisin > pix) {max = voisin; break;}
				}
			}
		}
		img_niv.at<uchar>(y,x) = (max == pix) ? max : img_niv.at<uchar>(y,x);
	}	
}

// Transformation 3 --------------------------

void effectuer_pelage_RDT (cv::Mat img_niv, int connexite)
{
    if (img_niv.type() != CV_8UC1) {
        std::cout << __func__ << ": format non géré :" << img_niv.type() << std::endl;
        return;
    }




}

// Appelez ici vos transformations selon affi
void effectuer_transformations (My::Affi affi, cv::Mat img_niv, int connexite)
{
    switch (affi) {
        case My::A_TRANS1 :
            effectuer_pelage_contours (img_niv, connexite);
            break;
        case My::A_TRANS2 :
            detecter_maximums_locaux (img_niv, connexite);
            break;
        case My::A_TRANS3 :
            effectuer_pelage_RDT (img_niv, connexite);
            break;
        default : ;
    }
}


//---------------------------- C A L L B A C K S ------------------------------

// Callback des sliders
void onZoomSlide (int pos, void *data)
{
    My *my = (My*) data;
    my->loupe.reborner (my->img_res1, my->img_res2);
    my->set_recalc(My::R_LOUPE);
}

void onSeuilSlide (int pos, void *data)
{
    My *my = (My*) data;
    my->set_recalc(My::R_SEUIL);
}


// Callback pour la souris
void onMouseEvent (int event, int x, int y, int flags, void *data)
{
    My *my = (My*) data;

    switch (event) {
        case cv::EVENT_LBUTTONDOWN :
            my->clic_x = x;
            my->clic_y = y;
            my->clic_n = 1;
            break;
        case cv::EVENT_MOUSEMOVE :
            // std::cout << "mouse move " << x << "," << y << std::endl;
            if (my->clic_n == 1) {
                my->loupe.deplacer (my->img_res1, my->img_res2, 
                    x - my->clic_x, y - my->clic_y);
                my->clic_x = x;
                my->clic_y = y;
                my->set_recalc(My::R_LOUPE);
            }
            break;
        case cv::EVENT_LBUTTONUP :
            my->clic_n = 0;
            break;
    }
}


void afficher_aide() {
    // Indiquez les transformations ici
    std::cout <<
        "Touches du clavier:\n"
        "   a    affiche cette aide\n"
        " hHlL   change la taille de la loupe\n"
        "   i    inverse les couleurs de src\n"
        "   o    affiche l'image src originale\n"
        "   s    affiche l'image src seuillée\n"
        "   1    affiche l'image pelée\n"
        "   2    affiche les maximums locaux (Modulo 16)\n"
        "   3    affiche l'image rétablie par distance inverse\n"
        "  esc   quitte\n"
    << std::endl;
}

// Callback "maison" pour le clavier
int onKeyPressEvent (int key, void *data)
{
    My *my = (My*) data;

    if (key < 0) return 0;        // aucune touche pressée
    key &= 255;                   // pour comparer avec un char
    if (key == 27) return -1;     // ESC pour quitter

    switch (key) {
        case 'a' :
            afficher_aide();
            break;
        case 'h' :
        case 'H' :
        case 'l' :
        case 'L' : {
            std::cout << "Taille loupe" << std::endl;
            int h = my->img_res2.rows, w = my->img_res2.cols; 
            if      (key == 'h') h = h >=  200+100 ? h-100 :  200;
            else if (key == 'H') h = h <= 2000-100 ? h+100 : 2000;
            else if (key == 'l') w = w >=  200+100 ? w-100 :  200;
            else if (key == 'L') w = w <= 2000-100 ? w+100 : 2000;
            my->img_res2 = cv::Mat(h, w, CV_8UC3);
            my->loupe.reborner(my->img_res1, my->img_res2);
            my->set_recalc(My::R_LOUPE);
          } break;
        case 'i' :
            std::cout << "Couleurs inversées" << std::endl;
            inverser_couleurs(my->img_src);
            my->set_recalc(My::R_SEUIL);
            break;
        case 'o' :
            std::cout << "Image originale" << std::endl;
            my->affi = My::A_ORIG;
            my->set_recalc(My::R_TRANSFOS);
            break;
        case 's' :
            std::cout << "Image seuillée" << std::endl;
            my->affi = My::A_SEUIL;
            my->set_recalc(My::R_SEUIL);
            break;
        case 'c' :
            my->connexite = 1 - my->connexite;
			if (my->connexite) {
            std::cout << "Nouvelle connexite : 8" << std::endl;
			}
			else {
            std::cout << "Nouvelle connexite : 4" << std::endl;
			}
            my->set_recalc(My::R_TRANSFOS);
            break;

        // Rajoutez ici des touches pour les transformations
        case '1' :
            std::cout << "Transformation : Pelage" << std::endl;
            my->affi = My::A_TRANS1;
            my->set_recalc(My::R_TRANSFOS);
            break;
        case '2' :
            std::cout << "Transformation 2" << std::endl;
            my->affi = My::A_TRANS2;
            my->set_recalc(My::R_TRANSFOS);
            break;
        case '3' :
            std::cout << "Transformation 3" << std::endl;
            my->affi = My::A_TRANS3;
            my->set_recalc(My::R_TRANSFOS);
            break;

        default :
            //std::cout << "Touche '" << char(key) << "'" << std::endl;
            break;
    }
    return 1;
}


//---------------------------------- M A I N ----------------------------------

void afficher_usage (char *nom_prog) {
    std::cout << "Usage: " << nom_prog
              << "[-mag width height] [-thr seuil] in1 out2" 
              << std::endl;
}

int main (int argc, char**argv)
{
    My my;
    char *nom_in1, *nom_out2, *nom_prog = argv[0];
    int zoom_w = 600, zoom_h = 500;

    while (argc-1 > 0) {
        if (!strcmp(argv[1], "-mag")) {
            if (argc-1 < 3) { afficher_usage(nom_prog); return 1; }
            zoom_w = atoi(argv[2]);
            zoom_h = atoi(argv[3]);
            argc -= 3; argv += 3;
        } else if (!strcmp(argv[1], "-thr")) {
            if (argc-1 < 2) { afficher_usage(nom_prog); return 1; }
            my.seuil = atoi(argv[2]);
            argc -= 2; argv += 2;
        } else break;
    }
    if (argc-1 != 2) { afficher_usage(nom_prog); return 1; }
    nom_in1  = argv[1];
    nom_out2 = argv[2];

    // Lecture image
    my.img_src = cv::imread (nom_in1, cv::IMREAD_COLOR);  // produit du 8UC3
    if (my.img_src.empty()) {
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

    // Création résultats
    my.img_res1 = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC3);
    my.img_res2 = cv::Mat(zoom_h, zoom_w, CV_8UC3);
    my.img_niv  = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC1);
    my.img_coul = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC3);
    my.loupe.reborner(my.img_res1, my.img_res2);

    cv::Mat img_gry; // Si on veut voir directement l'image pelée
    cv::cvtColor (my.img_src, img_gry, cv::COLOR_BGR2GRAY);
    cv::threshold (img_gry, my.img_niv, my.seuil, 255, cv::THRESH_BINARY);

    // Création fenêtre
    cv::namedWindow ("ImageSrc", cv::WINDOW_AUTOSIZE);
    cv::createTrackbar ("Zoom", "ImageSrc", &my.loupe.zoom, my.loupe.zoom_max, 
        onZoomSlide, &my);
    cv::createTrackbar ("Seuil", "ImageSrc", &my.seuil, 255, 
        onSeuilSlide, &my);
    cv::setMouseCallback ("ImageSrc", onMouseEvent, &my);

    cv::namedWindow ("Loupe", cv::WINDOW_AUTOSIZE);
    afficher_aide();

    // Boucle d'événements
    for (;;) {

        if (my.need_recalc(My::R_SEUIL) && my.affi != My::A_ORIG) 
        {
            // std::cout << "Calcul seuil" << std::endl;
            cv::Mat img_gry;
            cv::cvtColor (my.img_src, img_gry, cv::COLOR_BGR2GRAY);
            cv::threshold (img_gry, my.img_niv, my.seuil, 255, cv::THRESH_BINARY);
        }

        if (my.need_recalc(My::R_TRANSFOS))
        {
            // std::cout << "Calcul transfos" << std::endl;
            if (my.affi != My::A_ORIG) {
            	cv::Mat img_gry; // On effectue à nouveau le seuillage car l'image pelée en a besoin
            	cv::cvtColor (my.img_src, img_gry, cv::COLOR_BGR2GRAY);
            	cv::threshold (img_gry, my.img_niv, my.seuil, 255, cv::THRESH_BINARY);
                effectuer_transformations (my.affi, my.img_niv, my.connexite);
                representer_en_couleurs_vga (my.img_niv, my.img_coul);
            } else my.img_coul = my.img_src.clone();
        }

        if (my.need_recalc(My::R_LOUPE)) {
            // std::cout << "Calcul loupe puis affichage" << std::endl;
            my.loupe.dessiner_rect    (my.img_coul, my.img_res1);
            my.loupe.dessiner_portion (my.img_coul, my.img_res2);
            cv::imshow ("ImageSrc", my.img_res1);
            cv::imshow ("Loupe"   , my.img_res2);
        }
        my.reset_recalc();

        // Attente du prochain événement sur toutes les fenêtres, avec un
        // timeout de 15ms pour détecter les changements de flags
        int key = cv::waitKey (15);

        // Gestion des événements clavier avec une callback "maison" que l'on
        // appelle nous-même. Les Callbacks souris et slider sont directement
        // appelées par waitKey lors de l'attente.
        if (onKeyPressEvent (key, &my) < 0) break;
    }

    // Enregistrement résultat
    if (! cv::imwrite (nom_out2, my.img_coul))
         std::cout << "Erreur d'enregistrement" << std::endl;
    else std::cout << "Enregistrement effectué" << std::endl;

    return 0;
}

