#include <iostream>
#include <cstring>
#include <opencv2/opencv.hpp>

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
                dest.at<cv::Vec3b>(y,x)[0] = 0;
                dest.at<cv::Vec3b>(y,x)[1] = 0;
                dest.at<cv::Vec3b>(y,x)[2] = 0;
                continue;
            }
            dest.at<cv::Vec3b>(y,x)[0] = src.at<cv::Vec3b>(y0,x0)[0];
            dest.at<cv::Vec3b>(y,x)[1] = src.at<cv::Vec3b>(y0,x0)[1];
            dest.at<cv::Vec3b>(y,x)[2] = src.at<cv::Vec3b>(y0,x0)[2];
        }
    }
};


// Mes données transmises aux callbacks
class My {
  public:
    cv::Mat img_src, img_res1, img_res2;
    Loupe loupe;
    int flag1 = 1;
    int clic_x = 0;
    int clic_y = 0;
    int clic_n = 0;
};


// Calcul des images à afficher ; on ne modifie jamais img_src ici.
void calculer_images_resultat (My &my)
{
    my.loupe.dessiner_rect (my.img_src, my.img_res1);
    my.loupe.dessiner_portion (my.img_src, my.img_res2);
}


// Callback du slider
void onZoomSlide (int pos, void *data)
{
    My *my = (My*) data;
    my->loupe.reborner (my->img_res1, my->img_res2);
    my->flag1 = 1;          // recalculer et réafficher
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
                my->flag1 = 1;
            }
            break;
        case cv::EVENT_LBUTTONUP :
            my->clic_n = 0;
            break;
    }
}

// Callback "maison" pour le clavier
int onKeyPressEvent (int key, void *data)
{
    My *my = (My*) data;

    if (key < 0) return 0;        // aucune touche pressée
    key &= 255;                   // pour comparer avec un char
    if (key == 27) return -1;     // ESC pour quitter

    switch (key) {
        case ' ' :
            std::cout << "Touche espace" << std::endl;
            //dessiner_un_rect (my->img_src);
            my->flag1 = 1;
            break;
        default :
            std::cout << "Touche '" << char(key) << "'" << std::endl;
            break;
    }
    return 1;
}


void afficher_usage (char *nom_prog) {
    std::cout << "Usage: " << nom_prog
              << "  [-mag width height] in1 out2" 
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
    my.img_src = cv::imread (nom_in1, cv::IMREAD_COLOR);
    if (my.img_src.empty()) {
        std::cout << "Erreur de lecture" << std::endl;
        return 1;
    }

    // Création résultats
    my.img_res1 = cv::Mat(my.img_src.rows, my.img_src.cols, CV_8UC3);
    my.img_res2 = cv::Mat(zoom_h, zoom_w, CV_8UC3);
    my.loupe.reborner(my.img_res1, my.img_res2);

    // Création fenêtre
    cv::namedWindow ("ImageSrc", cv::WINDOW_AUTOSIZE);
    cv::createTrackbar ("Zoom", "ImageSrc", &my.loupe.zoom, my.loupe.zoom_max, 
        onZoomSlide, &my);
    cv::setMouseCallback ("ImageSrc", onMouseEvent, &my);

    cv::namedWindow ("Loupe", cv::WINDOW_AUTOSIZE);

    std::cout << "Pressez espace pour recalculer,\n" \
                 "        w pour enregistrer,\n" \
                 "        ESC pour quitter." << std::endl;

    // Boucle d'événements
    for (;;) {

        // On recalcule les images résultat et on réaffiche si le flag est à 1.
        if (my.flag1) {
            //std::cout << "Calcul images puis affichage" << std::endl;
            calculer_images_resultat (my);
            cv::imshow ("ImageSrc", my.img_res1);
            cv::imshow ("Loupe"   , my.img_res2);
            my.flag1 = 0;
        }

        // Attente du prochain événement sur toutes les fenêtres, avec un
        // timeout de 15ms pour détecter les changements de flags
        int key = cv::waitKey (15);

        // Gestion des événements clavier avec une callback "maison" que l'on
        // appelle nous-même. Les Callbacks souris et slider sont directement
        // appelées par waitKey lors de l'attente.
        if (onKeyPressEvent (key, &my) < 0) break;
    }

    // Enregistrement résultat sans le rectangle du zoom
    my.loupe.zoom = 0;
    calculer_images_resultat (my);
    if (! cv::imwrite (nom_out2, my.img_res1))
         std::cout << "Erreur d'enregistrement" << std::endl;
    else std::cout << "Enregistrement effectué" << std::endl;

    return 0;
}

