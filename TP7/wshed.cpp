/* TP de Ludovic Grosse

--> Pour lancer le programme : ./wshed nomImage

Quelques informations :
- Dans ce programme, j'implémente le Watershed de Vincent - Soille. A titre de comparaison, on peut aussi utiliser le WaterShed d'OpenCV.
- Il est possible d'inverser l'image pour obtenir selon les cas un watershed plus intéressant.
- Il est possible de faire varier la précision de mon watershed en modifant le nombre de pixels de l'image 
- Pour ne voir que le watershed sans l'image de fond : Commenter la ligne 351
*/

#include <opencv2/core/utility.hpp>
#include "opencv2/imgproc.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/highgui.hpp"
#include <queue> 
#include <cstdio>
#include <iostream>

using namespace cv;
using namespace std;

// ----------------------------------- MON WATERSHED -----------------------------------------------

// Classe pour récupérer les valeurs des pixels

class Table { 
	public:

	int precision;
	vector<vector<Point> > v;
	
	Table(int p) : v(256) {
	this -> precision = p ;}

	void remplir(cv::Mat img) {
		for (int y = 0; y < img.rows; y++)
		for (int x = 0; x < img.cols; x++)
		{
		    int i = img.at<uchar>(y,x) - img.at<uchar>(y,x)%precision; // On arrondi selon la precision
		   	v[i].push_back(Point(x,y));
		}
	}
};

// Fonction auxiliaire pour trouver les voisins d'un point

void trouve_voisins(Point p, vector<Point> & v, Mat img) 
{
	if (p.x != 0) v.push_back(Point(p.x - 1, p.y));
	if (p.x != img.cols - 1) v.push_back(Point(p.x + 1, p.y));
	if (p.y != 0) v.push_back(Point(p.x, p.y - 1));
	if (p.y != img.rows - 1) v.push_back(Point(p.x, p.y + 1));
}

// Mon watershed

void mon_watershed(Mat img_in, Mat img_out, int precision)
{
    if (img_in.type() != CV_8UC1) {
        cout << __func__ << ": format non géré :" << img_out.type() << endl;
        return;
    }
		
	// Initilisations 
	Mat img_d  = Mat(img_in.rows, img_in.cols, CV_8UC1); // Image pour les distances (d = éloigement du pixel au bassin le plus proche)
	int mask = -2; // Valeurs donnee aux pixels du niveau en cours de traitement
	int wshed = 0; // Pixels sur la lignes des eaux
	int current_label = 0; // Pour l'indexation des differents bassins
	int current_dist;
	
	vector <Point > voisins; // Contiendra la liste des voisins
	queue <Point > fifo; // FIFO pour parcourir des points choisis

    for (int y = 0; y < img_out.rows; y++) 
	for (int x = 0; x < img_out.cols; x++) { img_out.at<int>(y,x) = -1; img_d.at<uchar>(y,x) = 0; };

	Table sorted_pix = Table(precision); // On génère la liste des pixels triés
	sorted_pix.remplir(img_in);	
	
	for (int i = 0; i < 256; i++)
	{
		// Sélection d'un niveau de gris 

		for(unsigned int j = 0; j < sorted_pix.v[i].size(); j++) // On parcours nos pixels dans l'ordre de leur niveau de gris
		{
			Point p = sorted_pix.v[i][j]; // Tri des pixels selon leur niveau de gris
			img_out.at<int>(p.y,p.x) = mask;

			trouve_voisins(p, voisins, img_in);
			for (unsigned int k = 0; k < voisins.size(); k++) // On récupère les pixels d'un niveau de gris qui ont un voisin déjà étudié
			{
				if (img_out.at<int>(voisins[k].y,voisins[k].x) > 0 || img_out.at<int>(voisins[k].y,voisins[k].x) == wshed)
					{
						img_d.at<uchar>(p.y,p.x) = 1;
						fifo.push(p);
						break;
					}
			}
			voisins.clear();
		}

		// Prolongement des bassins existants
		
		current_dist = 1;
		Point fict_pix = Point(-1,-1); // Permet de faire boucler circulairement la fifo tant qu'elle est non vide
		fifo.push(fict_pix);
		while (true)
		{
			Point p = fifo.front();
			fifo.pop();
			if (p == fict_pix) { 
				if (fifo.empty()) break;
				fifo.push(fict_pix);
				current_dist ++;
				p = fifo.front();
				fifo.pop();
			}
			trouve_voisins(p, voisins, img_in); // On se concentre sur les points qui ont des voisins déjà traités
			for (unsigned int k = 0; k < voisins.size(); k++)
			{
				int etat_voisin = img_out.at<int>(voisins[k].y,voisins[k].x);
				int etat_point = img_out.at<int>(p.y,p.x);
				if (img_d.at<uchar>(voisins[k].y,voisins[k].x) < current_dist && (etat_voisin > 0 || etat_voisin == wshed)) { // Selection des voisins traités 
					if (etat_voisin > 0) { // Cas où le voisin est dans un bassin
						if (etat_point == mask || etat_point == wshed) { // Le point est indeterminé 
							img_out.at<int>(p.y,p.x) = etat_voisin;	// Propose l'appartenance au bassin du voisin
						}
						else if (etat_voisin != etat_point) { // Dans ce cas le point a déjà vu un voisin d'un autre bassin
							img_out.at<int>(p.y,p.x) = wshed; // Le point appartient alors a une frontiere
						}   		
					}
					else if (etat_point == mask) { // Cas où le voisin est un wshed et que le point est non traité
						img_out.at<int>(p.y,p.x) = wshed; // Première itération (Point non traité), on propose de prolonger la frontière
					}
				}
				else if (img_d.at<uchar>(voisins[k].y,voisins[k].x) == 0 && etat_voisin == mask/*0*/) { // Si le voisin est totalement inconnu
					fifo.push(voisins[k]); // Il a un voisin traité, et fera partie de la montée d'eau suivante, donc on l'ajoute
					img_d.at<uchar>(voisins[k].y,voisins[k].x) = current_dist + 1; // Il sera éloigné un peu plus du bassin
				}
			}
			voisins.clear();
		}

		// Création des nouveaux bassins

		for(unsigned int j = 0; j < sorted_pix.v[i].size(); j++) // On parcours à nouveau nos pixels dans l'ordre de leur niveau de gris
		{
			Point p = sorted_pix.v[i][j];
			img_d.at<uchar>(p.y,p.x) = 0; // On réinitialise les distances
			if (img_out.at<int>(p.y,p.x) == mask) { // Si le point appartient à un nouveau bassin (Non traité dans le cas précédent)
				current_label ++;
				fifo.push(p); // Pour traiter les voisins qui ont désormais un voisin traité (Le min du nouveau bassin) en leur attribuant le même label
				img_out.at<int>(p.y,p.x) = current_label;
				while (!fifo.empty())
				{
					Point p_voisin = fifo.front();
					fifo.pop();
					trouve_voisins(p_voisin, voisins, img_in);
					for (unsigned int k = 0; k < voisins.size(); k++) { // Traitement des points de la zone du nouveau minimum trouvé
						if (img_out.at<int>(voisins[k].y,voisins[k].x) == mask) {
							fifo.push(voisins[k]);
							img_out.at<int>(voisins[k].y,voisins[k].x) = current_label;
						}
					}
					voisins.clear();
				}
			}
		}		
	}
}

// ----------------------------------- GENERAL -----------------------------------------------

// Affichage informations

static void help()
{
    cout << "\nProgramme de démonstration de l'algorithme watershed d'OpenCV\n\n";
    cout <<
	    "- ECHAP : Quitte le programme\n"
        "- r : Restore l'image originale\n"
        "- i : Inverse les couleurs de l'image originale\n"
        "- ESPACE : Lance le watershed d'OpenCV\n" 
		"\t--> Cliquer d'abord sur l'image pour sélectionner les zones à segmenter\n"
        "- m : Lance mon watershed, puis :\n"
		"\t--> o : Diminue la précision\n"
		"\t--> p : Augmente la précision\n"
		"\t--> ECHAP : Quitte mon watershed\n";
}

Mat markerMask, img;
Point prevPt(-1, -1);

// Dessine sur l'image à l'écran pour sélectionner les zones à semgenter

static void onMouse( int event, int x, int y, int flags, void* )
{
    if( x < 0 || x >= img.cols || y < 0 || y >= img.rows )
        return;
    if( event == EVENT_LBUTTONUP || !(flags & EVENT_FLAG_LBUTTON) )
        prevPt = Point(-1,-1);
    else if( event == EVENT_LBUTTONDOWN )
        prevPt = Point(x,y);
    else if( event == EVENT_MOUSEMOVE && (flags & EVENT_FLAG_LBUTTON) )
    {
        Point pt(x, y);
        if( prevPt.x < 0 )
            prevPt = pt;
        line( markerMask, prevPt, pt, Scalar::all(255), 5, 8, 0 );
        line( img, prevPt, pt, Scalar::all(255), 5, 8, 0 );
        prevPt = pt;
        imshow("image", img);
    }
}

int main( int argc, char** argv )
{
    // Lecture image
	char *mat = argv[1];

    Mat img0  = imread (mat, IMREAD_COLOR);  
    if (img0.empty()) {
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

	// Initialisations
	Mat imgGray;
	help();
    namedWindow( "image", 1 );
    img0.copyTo(img);
    cvtColor(img, markerMask, COLOR_BGR2GRAY);
    cvtColor(markerMask, imgGray, COLOR_GRAY2BGR);
    markerMask = Scalar::all(0);
    imshow( "image", img );
    setMouseCallback("image", onMouse, 0);

    for(;;)
    {
		// Gestion des touches
        char c = (char) waitKey(0);
        if( c == 27 ) // Quitter
            break;
        if( c == 'r' ) // RAZ
        {
            markerMask = Scalar::all(0); // Réinitialisation markers
            img0.copyTo(img); // Réinitialisation image
            imshow( "image", img ); 
        }
        if( c == 'i' ) // Inversion image
        {
			bitwise_not (img0, img0);
			bitwise_not (img, img);
            imshow( "image", img0); 
		}
        if( c == ' ' )	 // Watershed
        {
			
	        int i, j, compCount = 0;
	        vector<vector<Point> > contours;
	        vector<Vec4i> hierarchy;

	        findContours(markerMask, contours, hierarchy, RETR_CCOMP, CHAIN_APPROX_SIMPLE); // Récupération des contours dans markeMask
	        if( contours.empty() ) // Si on a pas mis de markers on ne fait pas le watershed
	            continue;

	        Mat markers(markerMask.size(), CV_32S); // Matrice d'accueil du watershed
	        markers = Scalar::all(0);

	        int idx = 0;

	        for( ; idx >= 0; idx = hierarchy[idx][0], compCount++ )
	            drawContours(markers, contours, idx, Scalar::all(compCount+1), -1, 8, hierarchy, INT_MAX);
	        if( compCount == 0 )
	            continue;	
	        vector<Vec3b> colorTab;

	        for( i = 0; i < compCount; i++ ) // Génération aléatoire des couleurs des bassins
	        {
	            int b = theRNG().uniform(0, 255);
	            int g = theRNG().uniform(0, 255);
	            int r = theRNG().uniform(0, 255);
	            colorTab.push_back(Vec3b((uchar)b, (uchar)g, (uchar)r));
	        }

	        double t = (double)getTickCount(); // Pour le calcul du temps d'exécution
	        watershed( img0, markers); // WATERSHED
	        t = (double)getTickCount() - t;
	        printf( "Temps d'exécution : %gms\n", t*1000./getTickFrequency() );

	        Mat wshed(markers.size(), CV_8UC3); // Affichage de l'image en 8 bits

	        // Affichage de l'image
	        for( i = 0; i < markers.rows; i++ )
	            for( j = 0; j < markers.cols; j++ )
	            {
	                int index = markers.at<int>(i,j); // Chaque zone de l'image correspond à un cas d'index
					if (i%20 == 0) cout << " - " << index << endl ;
	                if( index == -1 ) // Contours
	                    wshed.at<Vec3b>(i,j) = Vec3b(255,255,255); // Les contours sont tracés en blanc
	                else if( index <= 0 || index > compCount )
	                    wshed.at<Vec3b>(i,j) = Vec3b(0,0,0);
	                else // Les bassins sont associés à une couleur
	                    wshed.at<Vec3b>(i,j) = colorTab[index - 1];
	            }
	        wshed = wshed*0.5 + imgGray*0.5; // On affiche l'image en transparence supperposé avec le watershed couleur
	        imshow( "watershed transform", wshed );
		}   
		else if (c == 'm')
		{
		int precision = 1; // Precision du watershed
			while(c == 'm' || c == 'o' || c == 'p')	 
			{
				Mat img_gry;
				cv::cvtColor (img0, img_gry, cv::COLOR_BGR2GRAY); // Passage de l'image source en nuance de gris

				Mat markers(img_gry.size(), CV_32SC1); // Matrice d'accueil du watershed
				markers = Scalar::all(0);

				double t = (double)getTickCount(); // Pour le calcul du temps d'exécution
				mon_watershed(img_gry, markers, precision); // MON WATERSHED
				t = (double)getTickCount() - t;
				printf("Temps d'exécution : %gms\n", t*1000./getTickFrequency());

	 			Mat wshed(markers.size(), CV_8UC3); // Affichage de l'image en 8 bits

				// Affichage de l'image
				for(int i = 0; i < markers.rows; i++ )
				    for(int j = 0; j < markers.cols; j++ )
				    {
						Point p(j,i);
						vector<Point> voisin;
						trouve_voisins(p, voisin, markers);

						for (unsigned int k = 0; k < voisin.size(); k++) // Permet d'identifier les watershed
						{
							if (markers.at<int>(p.y,p.x) != markers.at<int>(voisin[k].y,voisin[k].x) && markers.at<int>(voisin[k].y,voisin[k].x) != -1)
							{
								markers.at<int>(p.y,p.x) = -1;
								break;
							}
						}
						int index = markers.at<int>(i,j); // Chaque zone de l'image correspond à un cas d'index
						//if (i%20 == 0) cout << " - " << index << endl ;
				        if(index == -1) // Contours
				            wshed.at<Vec3b>(i,j) = Vec3b(255,255,255); // Les watershed sont tracés en blanc
				        else // Les bassins sont associés à une couleur
				            wshed.at<Vec3b>(i,j) = Vec3b((256 + index * 10)%256,(256 - index * 10)%256,(256 + (index + 1) * 5)%256);
				    }
				wshed = wshed*0.5 + imgGray*0.5; // On affiche l'image en transparence supperposé avec le watershed couleur
				imshow("watershed transform", wshed );	
				c = waitKey(0);
				destroyWindow("watershed transform");
				if (c == 'p') precision = (precision == 261) ? 261 : precision + 10 ; // Diminue la précision
				if (c == 'o') precision = (precision == 1) ? 1 : precision - 10 ; // Augmente la précision
			}
		}
	} 

    return 0;
}
