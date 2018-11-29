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

void representer_en_couleurs_vga (cv::Mat img_gry, cv::Mat img_coul)
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

    for (int y = 0; y < img_gry.rows; y++)
    for (int x = 0; x < img_gry.cols; x++)
    {
        int g = img_gry.at<uchar>(y,x);
        // Attention img_coul est en B, G, R -> inverser les canaux
        img_coul.at<cv::Vec3b>(y,x)[0] = couls[g][2];
        img_coul.at<cv::Vec3b>(y,x)[1] = couls[g][1];
        img_coul.at<cv::Vec3b>(y,x)[2] = couls[g][0];
    }
}


//----------------------------------- CLASSES -------------------------------------

class My {
  public:
    cv::Mat img_src, img_res1, img_res2, img_gry, img_coul;
    Loupe loupe;
    int clic_x = 0;
    int clic_y = 0;
    int clic_n = 0;

    enum Recalc { R_RIEN, R_LOUPE, R_TRANSFOS};
    Recalc recalc = R_TRANSFOS;

    void reset_recalc ()             { recalc = R_RIEN; }
    void set_recalc   (Recalc level) { if (level > recalc) recalc = level; }
    int  need_recalc  (Recalc level) { return level <= recalc; }

    // Rajoutez ici des codes A_TRANSx pour le calcul et l'affichage
    enum Affi { A_ORIG, A_TRANS1, A_TRANS2};
    Affi affi = A_ORIG;
};

class Table { // Pour récupérer les valeurs des pixels
	public:

	std::vector<std::vector < cv::Point > > v;
	
	Table() : v(256) {}

	void remplir(cv::Mat img) {
		for (int y = 0; y < img.rows; y++)
		for (int x = 0; x < img.cols; x++)
		{
		    int i = img.at<uchar>(y,x);
		   	v[i].push_back(cv::Point(x,y));
		}
	}
	
	// temporaire pour le debug
	void affiche() {
		for (int i = 0; i < v.size(); i++) 
		{	
			std::cout << "- " << i << ": " <<  v[i].size();
			std::cout << std::endl;
		}
	}
};


//----------------------- T R A N S F O R M A T I O N S -----------------------

// Fonction auxiliaire de cacul de voisins

void trouve_voisins(cv::Point p, std::vector <cv::Point > & v, cv::Mat img) 
{
	if (p.x != 0) v.push_back(cv::Point(p.x - 1, p.y));
	if (p.x != img.cols) v.push_back(cv::Point(p.x + 1, p.y));
	if (p.y != 0) v.push_back(cv::Point(p.x, p.y - 1));
	if (p.y != img.rows) v.push_back(cv::Point(p.x, p.y + 1));
}

// Transformation 1 - Mon watershed

void transformer (cv::Mat img_gry)
{
    if (img_gry.type() != CV_8UC1) {
        std::cout << __func__ << ": format non géré :" << img_gry.type() << std::endl;
        return;
    }
		
	cv::Mat img_i = img_gry.clone(); // Image qui servira de référence
	cv::Mat img_d  = cv::Mat(img_i.rows, img_i.cols, CV_8UC1); // Image des distances 
	int mask = -2; // Valeurs donnee aux pixels en cours de traitement
	int wshed = 0; // Pixels sur la lignes des eaux
	int current_label = 0; // Pour l'indexation des differents bassins
	int current_dist;
	
	std::vector <cv::Point > voisins; // Contiendra la liste des voisins
	std::queue <cv::Point > fifo; // FIFO pour parcourir des points choisis

    for (int y = 0; y < img_gry.rows; y++) 
	for (int x = 0; x < img_gry.cols; x++) { img_gry.at<uchar>(y,x) = -1; img_d.at<uchar>(y,x) = 0; }

	Table sorted_pix = Table(); // On génère la liste des pixels triés
	sorted_pix.remplir(img_i);	
	
	for (int i = 0; i < 256; i++)
	{
		for(int j = 0; j < sorted_pix.v[i].size(); j++) // On parcours nos pixels dans l'ordre de leur niveau de gris
		{
			cv::Point p = sorted_pix.v[i][j];
			img_gry.at<uchar>(p.y,p.x) = mask;

			trouve_voisins(p, voisins, img_i);
			for (int k = 0; k < voisins.size(); k++) // On récupère les pixels d'un niveau de gris qui ont un voisin déjà étudié
			{
				if (img_gry.at<uchar>(voisins[k].y,voisins[k].x) > 0 || img_gry.at<uchar>(voisins[k].y,voisins[k].x) == wshed)
					{
						img_d.at<uchar>(p.y,p.x) = 1;
						fifo.push(p);
						break;
					}
			}
			voisins.clear();
		}
		
		current_dist = 1;
		cv::Point fict_pix = cv::Point(-1,-1); // Permet de faire boucler circulairement la fifo tant qu'elle est non vide
		fifo.push(fict_pix);
		while (true)
		{
			cv::Point p = fifo.front();
			fifo.pop();
			if (p == fict_pix) { 
				if (fifo.empty()) break;
				fifo.push(fict_pix);
				current_dist ++;
				p = fifo.front();
				fifo.pop();
			}
			trouve_voisins(p, voisins, img_i); // On se concentre sur les points qui ont des voisins déjà traités
			for (int k = 0; k < voisins.size(); k++)
			{
				int etat_voisin = img_gry.at<uchar>(voisins[k].y,voisins[k].x);
				int etat_point = img_gry.at<uchar>(p.y,p.x);
				if (img_d.at<uchar>(voisins[k].y,voisins[k].x) < current_dist && (etat_voisin > 0 || etat_voisin == wshed)) { // Selection des voisins traités
					if (etat_voisin > 0) { // Cas où le voisin est dans un bassin
						if (etat_point == mask || etat_point == wshed) { // Si ???
							img_gry.at<uchar>(p.y,p.x) = etat_voisin;		
						}
						else if (etat_voisin != etat_point) { // Si le point a un label (!= wshed ou mask) différent d'un de ses voisins
							img_gry.at<uchar>(p.y,p.x) = wshed; // Dans ce cas le point appartient a une frontiere
						}   		
					}
					else if (etat_point == mask) { // Cas où le voisin est un wshed et que le point est non traité
						img_gry.at<uchar>(p.y,p.x) = wshed;
					}
				}
				else if (img_d.at<uchar>(voisins[k].y,voisins[k].x) == 0 && etat_voisin  == 0) { // Si le voisin est au même niveau de gris mais n'avait pas de voisins traités
					fifo.push(voisins[k]); // Il en a désormais un, donc on l'ajoute
					img_d.at<uchar>(voisins[k].y,voisins[k].x) = current_dist + 1; // On incrémente sa distance (d = le moment ou l'on doit traiter le point dans la liste) --> En l'occurence, il fait partie des points que l'on devra traiter dans les tours suivants : On travaille par bassin
				}
			}
			voisins.clear();
		}
		for(int j = 0; j < sorted_pix.v[i].size(); j++) // On parcours à nouveau nos pixels dans l'ordre de leur niveau de gris
		{
			cv::Point p = sorted_pix.v[i][j];
			img_d.at<uchar>(p.y,p.x) = 0; // On réinitialise les distances
			if (img_gry.at<uchar>(p.y,p.x) == mask) { // Si le point appartient à un nouveau bassin (Non traité dans le cas précédent)
				current_label ++;
				fifo.push(p); // Pour traiter les voisins qui ont désormais un voisin traité (Le min du nouveau bassin) en leur attribuant le même label
				img_gry.at<uchar>(p.y,p.x) = current_label;
				while (!fifo.empty())
				{
					cv::Point p_voisin = fifo.front();
					fifo.pop();
					trouve_voisins(p_voisin, voisins, img_i);
					for (int k = 0; k < voisins.size(); k++) { // Traitement des points de la zone du nouveau minimum trouvé
						if (img_gry.at<uchar>(voisins[k].y,voisins[k].x) == mask) {
							fifo.push(voisins[k]);
							img_gry.at<uchar>(voisins[k].y,voisins[k].x) = current_label;
						}
					}
				}
			}
		}
	}
	img_i = img_gry.clone();
}

// Transformation 2 - Watershed d'openCV

void watershed_machine (cv::Mat img_gry, cv::Mat img_src)
{
    if (img_gry.type() != CV_8UC1) {
        std::cout << __func__ << ": format non géré :" << img_gry.type() << std::endl;
        return;
    }
	cv::Mat img_aux = cv::Mat::zeros(img_src.rows, img_src.cols, CV_32SC1);
	cv::watershed(img_src, img_aux);
	img_aux.convertTo(img_gry, CV_8UC1);

	std::cout << "SRC: "<< std::endl;
	std::cout << "- 1: " << (int)img_src.at<cv::Vec3b>(0,0)[0] << std::endl;
	std::cout << "- 2: " <<  (int)img_src.at<cv::Vec3b>(50,50)[0] << std::endl;
	std::cout << "- 3: " <<  (int)img_src.at<cv::Vec3b>(100,100)[0] << std::endl << std::endl;


	std::cout << "AUX: "<< std::endl;
	std::cout << "- 1: " << (int)img_aux.at<uchar>(0,0) << std::endl;
	std::cout << "- 2: " << (int)img_aux.at<uchar>(50,50) << std::endl;
	std::cout << "- 3: " << (int)img_aux.at<uchar>(100,100) << std::endl << std::endl;	

	std::cout << "GRY: "<< std::endl;
	for (int y = 0; y < 10; y++)
	{
	for (int x = 0; x < 10; x++) std::cout << "- 1: " << (int)img_gry.at<uchar>(10*y,10*x) << std::endl;
	}
}

// Appelez ici vos transformations selon affi
void effectuer_transformations (My & my)
{
    switch (my.affi) {
        case My::A_TRANS1 :
            transformer (my.img_gry);
			break;
        case My::A_TRANS2 :
            watershed_machine(my.img_gry, my.img_src);
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
        "   o    affiche l'image src originale\n"
        "   1    affiche le watershed homemade\n"
		"   2    affiche le watershed d'openCV\n"
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
        case 'o' :
            std::cout << "Image originale" << std::endl;
            my->affi = My::A_ORIG;
            my->set_recalc(My::R_TRANSFOS);
            break;
        case '1' :
            std::cout << "Transformation 1" << std::endl;
            my->affi = My::A_TRANS1;
            my->set_recalc(My::R_TRANSFOS);
            break;
        case '2' :
            std::cout << "Transformation 2" << std::endl;
            my->affi = My::A_TRANS2;
            my->set_recalc(My::R_TRANSFOS);
            break;
        default :
            break;
    }
    return 1;
}


//---------------------------------- M A I N ----------------------------------

void afficher_usage (char *nom_prog) {
    std::cout << "Usage: " << nom_prog
              << "[-mag width height] in1 out2" 
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
    my.img_gry  = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC1);
    my.img_coul = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC3);
    my.loupe.reborner(my.img_res1, my.img_res2);

    // Création fenêtre
    cv::namedWindow ("ImageSrc", cv::WINDOW_AUTOSIZE);
    cv::createTrackbar ("Zoom", "ImageSrc", &my.loupe.zoom, my.loupe.zoom_max, 
        onZoomSlide, &my);
    cv::setMouseCallback ("ImageSrc", onMouseEvent, &my);

    cv::namedWindow ("Loupe", cv::WINDOW_AUTOSIZE);
    afficher_aide();

    // Boucle d'événements
    for (;;) {

        if (my.need_recalc(My::R_TRANSFOS))
        {
            // std::cout << "Calcul transfos" << std::endl;
            if (my.affi != My::A_ORIG) {
            	cv::cvtColor (my.img_src, my.img_gry, cv::COLOR_BGR2GRAY);
                effectuer_transformations (my);
                representer_en_couleurs_vga (my.img_gry, my.img_coul);
				cv::imshow ("Test1", my.img_gry);
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

