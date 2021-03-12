// ----- Projecto: Simulador de Campos Eléctricos (SimCamPel) -----
//
// Por: Gonçalo Vaz e Nuno Fernandes

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <float.h>
#include <stdint.h>
#include <gtk/gtk.h>

#define VERSION "1.032"

#define INITIALVALUES { .state = 0, .numtq = 0, .numq = 0, .qs = NULL, .tqs = NULL, .qedit = SIZE_MAX }
//Coisas que convenham ser inicializadas...

#define K 8.9875517873681764e9
//A constante de Coulomb (para o cálculo da força eléctrica).
#define qe 1.6021766208e-19
//A carga elementar (para questões diversas de mudança de unidades).
#define SIZEOBSERVUNIV 9e26
//Estimativa, vagamente por excesso, do tamanho do Universo observável.
#define DIFFSCALESTRANGE 1e9
//Valor aproximado a partir do qual o tamanho relativo dos objectos a desenhar provoca efeitos estranhos...
#define ABSOLUTESTRANGENESS 1e-16
//Valor aproximado a partir do qual, por questões diversas de precisão dos floats, as coisas ficam estranhas...

#define ALMOSTZERO 1e-100
//Um valor suficientemente baixo para as comparações.
#define TIMEINTERVAL 0.01
//O intervalo de tempo (em segundos) entre cada novo desenho.
#define UUUUUUUO 254
//O número decimal correspondente a 11111110.
#define UUUOOOOU 225
//O número decimal correspondente a 11100001.
#define OOOUUUUU 31
//O número decimal correspondente a 00011111.
#define BEGINMU1 -62
#define ENDMU1 -75
//Um mu em UTF-8.
#define BEGINMU2 -50
#define ENDMU2 -68
//Outro mu em UTF-8.
#define BEGINANGS1 -61
#define ENDANGS1 -123
//Um Angstrom em UTF-8.
#define BEGINANGS2 -30
#define MIDDLEANGS2 -124
#define ENDANGS2 -85
//Outro Angstrom em UTF-8.

#define SIM_NUMELEM (12 + data->numq + data->numtq)
//O número de elementos a ler sempre que se abre uma simulação.
#define CONFIG_NUMELEM (8)
//O número de elementos a ler sempre que se abre uma configuração.



#define LASTSTATE "state.last"
//O ficheiro que guarda o último estado do programa.
#define CONFIG "simcampel.config"
//O ficheiro que guarda as configurações.
#define ICON "simcampel.png"
//Um ícone para a janela principal...
#define PLAYPAUSE "playpause.png"
//Um botão de fazer play e fazer pause...

/* ---------------------------------------------------------------- */
/* ----------------------- STRUCTS E AFINS ------------------------ */
/* ---------------------------------------------------------------- */

typedef struct {
        double x, y, vx, vy, m, r, q;
        //Coordenadas, velocidades, massa, raio e carga.
        GdkRGBA color;
        //Cores da carga (para propósitos estéticos...).
        char name[32];
         //Para se poder dar um nome à carga...
} charge;


typedef struct {
        double x, y, vx, vy;
        //Coordenadas e velocidades.
} testcharge;


typedef struct {
        double x, y, len, ang;
        //Coordenadas, comprimento e ângulo com a horizontal.
} vect;


//Indica os vários contextos da statusbar.
enum sbcontexts {NORMAL = 0, WARNINGS, CRITICAL, SBC_LAST};


//Indica as duas formas de fazer reset aos dados...
typedef enum resetmode {RES_SIM = 1, RES_CONFIG = 2} resetmode;


//Indica os diversos modos de fazer a interpretação das unidades...
typedef enum siconvertmode {PREFIXES = 0, LENGTH, TIME, VELOCITY, MASS, ELECTRIC_CHARGE} siconvertmode;


//Indica se se pretende mais informação sobre o programa ou não.
enum debugmodes {NOPE = 0, SURE};
//               00000000  00000001

#define check_debug(x) ((x) & SURE)


//Indica as várias formas de desenhar.
enum drawmodes {NONE = 0, VECTOR_DIRS = 2, FULL_VECTORS = 4, drmd_cmp = 6};
//              00000000    00000010          00000100       00000110

#define check_drawmode(x) ((x) & drmd_cmp)


//Indica os vários modos de representar o movimento.
enum playmodes {STOPPED = 0, ONLYTEST = 8, ALL = 16, plmd_cmp = 24};
//               00000000     00001000     00010000   00011000

#define check_playmode(x) ((x) & plmd_cmp)


//Indica se o utilizador quer guardar o último estado do programa.
enum savelast {NEVER = 0, ALWAYS = 32};
//             00000000     00100000

#define check_savelast(x) ((x) & ALWAYS)

//Indica se se pretende ver colisões das cargas de prova.
//O programa ficará a modos que mais lento se se quiser...
enum tqcoll {NEIN = 0, JA = 64};
//           00000000  01000000

#define check_tqcoll(x) ((x) & JA)


typedef struct {
        char state;
//O estado do programa.
        double timescale, scale, vectadjust;
//A escala de visualização e outra para os vectores.
        double scaleadjvalue;
//Para ajustar o valor representado da escala...
        double offsetx, offsety;
//Offsets para o x e para o y, para se deslocar a visualização.
        double minx, maxx, miny, maxy;
//Valores mínimos e máximos para as posições.
        unsigned char numvects;
//O número de vectores por lado (char para não ser demasiado grande...)
        size_t numq, qedit;
//O número de cargas, a carga a ser editada agora.
        charge *qs;
//Um array contendo as várias cargas colocadas.
        testcharge *tqs;
//Um array contendo unicamente cargas de prova.
        size_t numtq;
//O número de cargas de prova.
        GdkRGBA tqcolor;
//Cor das cargas de prova.
        double tqr, tqq, tqm;
//Propriedades das cargas de prova.
        GdkRGBA vectcolor;
//Cor dos vectores.
        gint to_motion, to_clean;
//Timeouts, para parar todas as funções aquando da saída.
        GtkWidget *mainwindow, *mainvbox, *sbframe, *statusbar,
                  *mainhbox, *leftvbox, *rightvbox, *rvbframe,
                  *lvbframe, *bvbox, *bframe,*daframe,
                  *drawarea, *i_label, *i_frame, *plmd_scale,
                  *plmdvbox, *plhbox, *playbutton, *tscale_spbutton,
                  *tse_label, *tse_hbox, *f_saver, *f_loader,
                  *uparrow, *downarrow, *leftarrow, *rightarrow,
                  *uabox, *lrbox, *dabox, *vis_scale,
                  *vs_title, *vs_label, *vs_box, *offx_entry,
                  *offx_label, *offy_entry, *offy_label, *off_box,
                  *minx_entry, *minx_label, *miny_entry, *miny_label,
                  *maxx_entry, *maxx_label, *maxy_entry, *maxy_label,
                  *minmaxbox, *quitbutton, *qbbox, *resetbutton,
                  *rbbox, *optionbutton, *obbox, *savebutton,
                  *sbbox, *loadbutton, *lbbox, *helpbutton,
                  *hbbox, *bbox1, *bbox2, *bbox3, *chargeditorbox,
                  *chargeditorframe, *ce_label, *ce_combobox, *ce_colorlabel,
                  *ce_colorbutton, *ce_mentry, *ce_mlabel, *ce_qentry,
                  *ce_qlabel, *ce_xentry, *ce_xlabel, *ce_yentry,
                  *ce_ylabel, *ce_vxentry, *ce_vxlabel, *ce_vyentry,
                  *ce_vylabel, *ce_namentry, *ce_namelabel, *ce_rentry,
                  *ce_rlabel, *addq_label, *addq_entry, *addq_inbox,
                  *addq_hbox, *optiondialog, *colorbtq, *cbtq_label,
                  *colorbvect, *cbvect_label, *tqr_entry, *tqr_label,
                  *tqm_entry, *tqm_label, *tqq_entry, *tqq_label,
                  *numvects_sb, *numvects_label, *vadj_scale, *vadj_label,
                  *odhbox1, *odhbox2, *odhbox3, *odhbox4,
                  *odhbox5, *odhbox6, *odhbox7, *odhbox8,
                  *mbox1, *mbox2, *vectmodes_scale, *tqc_b,
                  *sl_b, *ce_finallabel, *ce_poshbox, *ce_velhbox,
                  *ce_xvbox, *ce_yvbox, *ce_vxvbox, *ce_vyvbox,
                  *ce_delchargbutton;
//Uma acumulação enorme de widgets...

} pdata;


/* Convém frisar-se que se tentou, dentro do género, evitar ao máximo usar pointers em pdata,
   de modo que se possa mais facilmente restaurar estados passados, com menos leituras
   separadas (e com menos alocações dinâmicas de memória).                                   */


/* ---------------------------------------------------------------- */
/* ---------------------------- CSS ------------------------------- */
/* ---------------------------------------------------------------- */


/* Retirado de Gtk3_CssAux.c, para facilitar a compilação. */

void create_style_from_file (gchar *cssFile)
{
  gsize   bytes_written, bytes_read;
  GError *error = 0;
  gchar  *fileUTF8 ;

  GtkCssProvider *provider = gtk_css_provider_new ();
  GdkDisplay *display = gdk_display_get_default ();
  GdkScreen *screen = gdk_display_get_default_screen (display);
  gtk_style_context_add_provider_for_screen (screen, GTK_STYLE_PROVIDER(provider),
					     GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  fileUTF8 = g_filename_to_utf8 (cssFile, strlen(cssFile), &bytes_read, &bytes_written, &error);
  gtk_css_provider_load_from_path (provider, fileUTF8, NULL);
  free (fileUTF8);
  g_object_unref (provider);
}

/* Doravante, já não retirado de lado algum. */


/* ---------------------------------------------------------------- */
/* ------------------------ UTILIDADES ---------------------------- */
/* ---------------------------------------------------------------- */


/*
#define sign(x)                                   \
( { __typeof__(x) _x = (x);                       \
   ((_x) > ALMOSTZERO) - ((_x) < ALMOSTZERO); } )

#define max(x,y)                     \
( { __typeof__(x) _x = (x);          \
    __typeof__(y) _y = (y);          \
    ((_x) > (_y) ? (_x) : (_y)); } ) \

#define min(x,y)                     \
( { __typeof__(x) _x = (x);          \
    __typeof__(y) _y = (y);          \
    ((_x) < (_y) ? (_x) : (_y)); } ) \

  Estas definições de macros permitiriam
  poupar operações, posto que não haveria
  múltiplas avaliações dos argumentos.
  Porém, dado que __typeof__ é uma extensão
  GNU do C, em prol da portabilidade, acabou
  por se fazer as funções que seguem abaixo,
  que, por serem funções, acabam por ocupar
  um tanto-nada mais de memória do que se
  fosse tudo feito por intermédio de macros.
  Adicionalmente, depois das funções, seguem
  macros que permitem avaliar mais facilmente
  as coisas, se se der o caso de os argumentos
  serem garantidamente simples.
                                                        */

double sign(double x)
{
    return ((x > ALMOSTZERO) - (x < ALMOSTZERO));
}

double max(double x, double y)
{
    return (x > y ? x : y);
}

double min(double x, double y)
{
    return (x < y ? x : y);
}

#define simple_sign(x) (((x) > ALMOSTZERO) - ((x) < ALMOSTZERO))

#define simple_max(x,y) ((x) > (y) ? (x) : (y))

#define simple_min(x,y) ((x) < (y) ? (x) : (y))

#define randouble(min, max) (((double) rand()/((double) RAND_MAX)) * ((max) - (min)) + (min))
//Dá double aleatório no intervalo [min, max].

#define randsignal() (rand() & 1 ? 1 : -1)
//Dá sinal aleatório.


gboolean cleanstatusbar (GtkWidget *sb);
/* Protótipo da função (que está declarada
   juntamente com os restantes timeouts), para
   se poder fazer referência a ela nestas funções
   de utilidade.                                  */

vect calc_el(double x, double y, double q2, pdata *data, gboolean stop)
//Calcula o campo num dado ponto, para uma dada carga não-nula.
{
    size_t i;
    double delx, dely, dist, inten, tang;
    vect r = {0.0, 0.0, 0.0, 0.0};
    for (i=0; i < data->numq; ++i)
    {
        delx = (x - ((data->qs[i]).x))*simple_sign((data->qs[i]).q)*simple_sign(q2);
        dely = (y - ((data->qs[i]).y))*simple_sign((data->qs[i]).q)*simple_sign(q2);
        dist = sqrt(delx * delx + dely * dely);
        if (dist < (data->qs[i]).r)
        {
            if (stop)
            {
                return (vect) {0.0, 0.0, 0.0, 0.0};
            }
            continue;
        }
        inten = K*fabs(q2)*fabs((data->qs[i]).q)/dist;
        if (inten < ALMOSTZERO)
        {
            continue;
        }
        tang = dely/delx;
        r.len += inten;
        if(isinf(tang))
        {
            r.y += simple_sign(dely)*inten;
        }
        else if(fabs(tang) < ALMOSTZERO)
        {
            r.x += simple_sign(delx)*inten;
        }
        else
        {
            r.x += simple_sign(delx)*inten/sqrt(tang * tang + 1.0);
            r.y += simple_sign(dely)*inten*fabs(tang)/sqrt(tang * tang + 1.0);
        }
    }
    r.ang = atan2(r.y,r.x);
    return r;
}

void vect_to_size(vect *v, double sz)
//Põe um vector do tamanho desejado.
{
    (v->x) = sz*cos(v->ang);
    (v->y) = sz*sin(v->ang);
    (v->len) = sz;
}

void draw_vect(vect *v, double xo, double yo, cairo_t *cr)
//Desenha um vector, admitidamente válido.
{
    cairo_new_path(cr);
    cairo_move_to(cr, xo, yo);
    cairo_rel_line_to(cr, v->x, v->y);
    cairo_rel_line_to(cr, 0.2*v->len*cos(v->ang + 3*M_PI_4), 0.2*v->len*sin(v->ang + 3*M_PI_4));
    cairo_move_to(cr, xo + v->x, yo + v->y);
    cairo_rel_line_to(cr, 0.2*v->len*cos(v->ang - 3*M_PI_4), 0.2*v->len*sin(v->ang - 3*M_PI_4));
    cairo_stroke(cr);
}

void create_test_charge (double xo, double yo, pdata *data)
//Cria uma carga de prova, alocando tudo o que é necessário.
{
    (data->tqs) = (testcharge *) realloc(data->tqs, (data->numtq + 1)*sizeof(testcharge));
    if (!(data->tqs))
    {
        if (check_debug(data->state))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS,
                                "Erro na alocação das cargas de prova!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
        return;
    }
    data->tqs[data->numtq] = (testcharge) {xo, yo, 0.0, 0.0};
    ++(data->numtq);
    gtk_widget_queue_draw(data->drawarea);
}

void destroy_test_charge (size_t place, pdata *data)
//Destrói uma carga de prova e actualiza devidamente o array de cargas de prova.
{
    if (place + 1 != data->numtq)
    {
        data->tqs[place] = data->tqs[data->numtq - 1];
    }
    --(data->numtq);
    data->tqs=(testcharge *) realloc(data->tqs, (data->numtq)*sizeof(testcharge));
    if (!(data->numtq))
    {
        data->tqs = NULL;
    }
}

void qedit_update (pdata *data, size_t newqedit)
//Actualiza as informações relativamente à carga em edição.
{
    char t[50];
    data->qedit = newqedit;
    if (data->qedit == SIZE_MAX)
    {
        gtk_container_foreach(GTK_CONTAINER(data->chargeditorbox), (GtkCallback) gtk_widget_hide, NULL);
        gtk_widget_show(data->ce_combobox);
        gtk_widget_show(data->ce_label);
        return;
    }
    else
    {
        gtk_widget_show_all(data->chargeditorbox);
        gtk_widget_hide(data->ce_label);
    }
    size_t a = (size_t) gtk_combo_box_get_active(GTK_COMBO_BOX(data->ce_combobox)) - 1;
    if ( a != data->qedit)
    {
        gtk_combo_box_set_active(GTK_COMBO_BOX(data->ce_combobox), data->qedit + 1);
    }
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->ce_colorbutton), &data->qs[data->qedit].color);
    if (!gtk_widget_is_focus(data->ce_xentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].x);
        gtk_entry_set_text(GTK_ENTRY(data->ce_xentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_yentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].y);
        gtk_entry_set_text(GTK_ENTRY(data->ce_yentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_vxentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].vx);
        gtk_entry_set_text(GTK_ENTRY(data->ce_vxentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_vyentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].vy);
        gtk_entry_set_text(GTK_ENTRY(data->ce_vyentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_rentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].r);
        gtk_entry_set_text(GTK_ENTRY(data->ce_rentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_mentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].m);
        gtk_entry_set_text(GTK_ENTRY(data->ce_mentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_qentry))
    {
        sprintf(t, "%11.3lE", data->qs[data->qedit].q);
        gtk_entry_set_text(GTK_ENTRY(data->ce_qentry), t);
    }
    if (!gtk_widget_is_focus(data->ce_namentry))
    {
        gtk_entry_set_text(GTK_ENTRY(data->ce_namentry), data->qs[data->qedit].name);
    }
}

gboolean create_charge(pdata *data, double xo, double yo, double m, double r, double q, GdkRGBA color, char name[32])
//Cria uma carga normal.
{
    char t[30];
    (data->qs) = (charge *) realloc(data->qs, (data->numq + 1)*sizeof(charge));
    if (!(data->qs))
    {
        if (check_debug(data->state))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS,
                                "Erro na alocação das cargas!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
        return FALSE;
    }
    data->qs[data->numq] = (charge) {xo, yo, 0.0, 0.0, m, r, q, color};
    if (strcmp(name, "\n"))
    {
        sprintf(t,"%u: '%s'", data->numq + 1, name);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(data->ce_combobox), NULL, t);
        strcpy(data->qs[data->numq].name, name);
    }
    else
    {
        sprintf(t, "%u: Carga Anónima", data->numq + 1);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(data->ce_combobox), NULL, t);
        strcpy(data->qs[data->numq].name, "Carga Anónima");
    }
    ++(data->numq);
    if (data->numq > 1000)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS, "É bem provável que, face a este"
                            " número de cargas, o programa fique mais lento. Tenha cautela..."          );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
    }
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

void destroy_charge (size_t place, pdata *data)
//Destrói uma carga normal.
{
    char t[50];
    if (data->qedit == place)
    {
        qedit_update(data, SIZE_MAX);
    }
    if (place + 1 != data->numq)
    {
        gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(data->ce_combobox), place + 1);
        data->qs[place] = data->qs[data->numq - 1];
        sprintf(t, "%u: '%s'", place + 1, (data->qs[place]).name);
        gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(data->ce_combobox), place + 1, NULL, t);
    }
    gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(data->ce_combobox), data->numq);
    --(data->numq);
    data->qs=(charge *) realloc(data->qs, (data->numq)*sizeof(charge));
    if (!(data->numq))
    {
        data->qs = NULL;
    }
    gtk_widget_queue_draw(data->drawarea);
}

void q_coll_detect (pdata *data, size_t i)
//Trata de todas as questões de colisões e afastamentos de cargas.
{
    size_t j;
    double t1, t2, t3, t4;
    for (j = 0; j < data->numq; ++j)
    {
        if (j == i)
        {
            continue;
        }
        t1 = (data->qs[i]).x - (data->qs[j]).x;
        t2 = (data->qs[i]).y - (data->qs[j]).y;
        t3 = sqrt(t1 * t1 + t2 * t2);
        if(t3 < ((data->qs[i]).r + (data->qs[j]).r))
        {
            t4 = (data->qs[i]).vx; //Temporário.

            (data->qs[i]).vx = ( ((data->qs[j]).m*(2.0*(data->qs[j]).vx - (data->qs[i]).vx) +
                                (data->qs[i]).m*(data->qs[i]).vx)/((data->qs[i]).m + (data->qs[j]).m) );

            (data->qs[j]).vx = ( ((data->qs[i]).m*(2.0*t4 - (data->qs[j]).vx) +
                                (data->qs[j]).m*(data->qs[j]).vx)/((data->qs[j]).m + (data->qs[i]).m) );

            t4 = (data->qs[i]).vy; //Temporário.

            (data->qs[i]).vy = ( ((data->qs[j]).m*(2.0*(data->qs[j]).vy - (data->qs[i]).vy ) +
                                (data->qs[i]).m*(data->qs[i]).vy)/((data->qs[i]).m + (data->qs[j]).m) );

            (data->qs[j]).vy = ( ((data->qs[i]).m*(2.0*t4 - (data->qs[j]).vy) +
                                (data->qs[j]).m*(data->qs[j]).vy)/((data->qs[j]).m + (data->qs[i]).m) );
            t4 = -t2/t1;
            t3 = ((data->qs[i]).r + (data->qs[j]).r - t3)/2.0;
            if(isinf(t4))
            {
                (data->qs[i]).y += simple_sign(t2)*t3;
            }
            else if(fabs(t4) < ALMOSTZERO)
            {
                (data->qs[i]).x += simple_sign(t2)*t3;
            }
            else
            {
                (data->qs[i]).x += simple_sign(t1)*t3/sqrt(t4 * t4 + 1.0);
                (data->qs[i]).y += simple_sign(t2)*t3*fabs(t4)/sqrt(t4 * t4 + 1.0);
            }
            break;
            //Vamos eliminar a hipótese de colidirem três ou mais simultaneamente, para simplificar...
        }
    }
    if ((data->qs[i]).x - (data->qs[i]).r < data->minx)
    {
        (data->qs[i]).vx = -(data->qs[i]).vx;
        (data->qs[i]).x = data->minx + (data->qs[i]).r;
    }
    else if ((data->qs[i]).x + (data->qs[i]).r > data->maxx)
    {
        (data->qs[i]).vx = -(data->qs[i]).vx;
        (data->qs[i]).x = data->maxx - (data->qs[i]).r;
    }
    if ((data->qs[i]).y - (data->qs[i]).r < data->miny)
    {
        (data->qs[i]).vy  = -(data->qs[i]).vy;
        (data->qs[i]).y = data->miny + (data->qs[i]).r;
    }
    else if ((data->qs[i]).y + (data->qs[i]).r > data->maxy)
    {
        (data->qs[i]).vy = -(data->qs[i]).vy;
        (data->qs[i]).y = data->maxy - (data->qs[i]).r;
    }
}

void tq_coll_detect (pdata *data, size_t i)
//Detecta (opcionalmente) colisões de cargas de prova.
{
    size_t j;
    double t1, t2, t3, t4;
    for (j = 0; j < data->numq; ++j)
    {
        t1 = (data->tqs[i]).x - (data->qs[j]).x;
        t2 = (data->tqs[i]).y - (data->qs[j]).y;
        t3 = data->tqr * data->scale + (data->qs[j]).r;
        if(t1 * t1 + t2 * t2 < t3 * t3)
        {
            (data->tqs[i]).vx = -(data->tqs[i]).vx;
            (data->tqs[i]).vy = -(data->tqs[i]).vx;
            return;
        }
    }
    for (j = 0; j < data->numtq; ++j)
    {
        if (j == i)
        {
            continue;
        }
        t1 = (data->tqs[i]).x - (data->tqs[j]).x;
        t2 = (data->tqs[i]).y - (data->tqs[j]).y;
        t3 = sqrt(t1 * t1 + t2 * t2);
        t4 = 2.0 * data->tqr * data->scale;
        if(t3 < t4)
        {
            t4 = (data->tqs[i]).vx; //Temporário.
            (data->tqs[i]).vx = (data->tqs[j]).vx;
            (data->tqs[j]).vx = t4;
            t4 = (data->tqs[i]).vy; //Temporário.
            (data->tqs[i]).vy = (data->tqs[j]).vy;
            (data->tqs[j]).vy = t4;
            t4 = -t2/t1;
            t3 = (2.0 * data->tqr * data->scale - t3)/2.0;
            if(isinf(t4))
            {
                (data->tqs[i]).y += simple_sign(t2)*t3;
            }
            else if(fabs(t4) < ALMOSTZERO)
            {
                (data->tqs[i]).x += simple_sign(t2)*t3;
            }
            else
            {
                (data->tqs[i]).x += simple_sign(t1)*t3/sqrt(t4 * t4 + 1.0);
                (data->tqs[i]).y += simple_sign(t2)*t3*fabs(t4)/sqrt(t4 * t4 + 1.0);
            }
            return;
        }
    }
}

gboolean tq_edge_detect (pdata *data, size_t i, double wmin, double hmin, double wmax, double hmax)
//Apaga a carga de teste caso esteja fora do ecrã. Os argumentos são as dimensões.
{
    if ( (data->tqs[i]).x < wmin || (data->tqs[i]).y < hmin ||
         (data->tqs[i]).x > wmax || (data->tqs[i]).y > hmax    )

    {
        destroy_test_charge(i, data);
        return TRUE;
    }
    return FALSE;
}

void offset_change (double x_new, double y_new, pdata *data)
//Trata das alterações do offset e da actualização das respectivas entries.
{
    char t[25];
    data->offsetx = x_new;
    if (!gtk_widget_is_focus(data->offx_entry))
    {
        sprintf(t,"%11.3lE", data->offsetx);
        gtk_entry_set_text(GTK_ENTRY(data->offx_entry), t);
    }
    data->offsety = y_new;
    if (!gtk_widget_is_focus(data->offy_entry))
    {
        sprintf(t,"%11.3lE", data->offsety);
        gtk_entry_set_text(GTK_ENTRY(data->offy_entry), t);
    }
}

void update_scale(double new_scale, pdata *data, double rel_x, double rel_y)
//Actualiza a escala de visualização e todos os widgets associados.
{
    static char said = 0;
    if (new_scale > SIZEOBSERVUNIV)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Isso é maior que o diâmetro do"
                            " Universo observável! Não exageremos..."                                );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        update_scale(SIZEOBSERVUNIV, data, rel_x, rel_y);
        return;
    }
    else if (new_scale < ABSOLUTESTRANGENESS)
    {
        if (!said)
        {
            gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Dada esta nova escala, há "
                                "fortes possibilidades de surgirem efeitos estranhos. Não se assuste!" );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            said = 1;
        }
        return;
    }
    double lo = floor(log10(new_scale));
    char t[50];
    int i;
    said = 0;
    data->scaleadjvalue = pow(10, lo);
    offset_change( data->offsetx + rel_x*data->scale - rel_x*new_scale,
                   data->offsety + rel_y*data->scale - rel_y*new_scale, data );
    data->scale = new_scale;

    gtk_range_set_value(GTK_RANGE(data->vis_scale), new_scale/data->scaleadjvalue);
    sprintf(t, "x 10<sup><small>%.0lf</small></sup> m/px", lo);
    gtk_label_set_markup(GTK_LABEL(data->vs_label), t);
    gtk_scale_clear_marks(GTK_SCALE(data->vis_scale));
    for (i = 1; i < 10; ++i)
    {
        sprintf(t, "<small>%d x 10<sup><small>%.0lf</small></sup></small>", i, lo);
        gtk_scale_add_mark(GTK_SCALE(data->vis_scale), (double) i, GTK_POS_TOP, t);
    }
    sprintf(t, "<small>1 x 10<sup><small>%.0lf</small></sup></small>", lo + 1);
    gtk_scale_add_mark(GTK_SCALE(data->vis_scale), 10.0, GTK_POS_TOP, t);
}

void update_minmax_display (pdata *data)
//Actualiza as entries de min x, min y, max x e max y.
{
    size_t i;
    char t[25];
    for(i = 0; i < data->numq; ++i)
    {
        /* Os 0.01 por que se multiplicam as velocidades
           não são inteiramente correctos, de um ponto
           de vista estritamente físico, mas a ideia
           é que aproximar o limite vai "empurrar" as
           cargas mais para dentro...                    */
        if ((data->qs[i]).x + (data->qs[i]).r > data->maxx)
        {
            (data->qs[i]).vx -= 0.01*fabs((data->qs[i]).vx)/(data->qs[i]).m;
            data->maxx = (data->qs[i]).x + (data->qs[i]).r;
        }
        else if ((data->qs[i]).x - (data->qs[i]).r < data->minx)
        {
            (data->qs[i]).vx += 0.01*fabs((data->qs[i]).vx)/(data->qs[i]).m;
            data->minx = (data->qs[i]).x - (data->qs[i]).r;
        }
        if ((data->qs[i]).y + (data->qs[i]).r > data->maxy)
        {
            (data->qs[i]).vy -= 0.01*fabs((data->qs[i]).vy)/(data->qs[i]).m;
            data->maxy = (data->qs[i]).y + (data->qs[i]).r;
        }
        else if ((data->qs[i]).y - (data->qs[i]).r < data->miny)
        {
            (data->qs[i]).vy += 0.01*fabs((data->qs[i]).vy)/(data->qs[i]).m;
            data->miny = (data->qs[i]).y - (data->qs[i]).r;
        }
    }
    if (data->minx < -SIZEOBSERVUNIV)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Isso é maior que o diâmetro do"
                            " Universo observável! Não exageremos..."                                );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        data->minx = -SIZEOBSERVUNIV;
    }
    if (data->miny < -SIZEOBSERVUNIV)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Isso é maior que o diâmetro do"
                            " Universo observável! Não exageremos..."                                );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        data->miny = -SIZEOBSERVUNIV;
    }
    if (data->maxx > SIZEOBSERVUNIV)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Isso é maior que o diâmetro do"
                            " Universo observável! Não exageremos..."                                );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        data->maxx = SIZEOBSERVUNIV;
    }
    if (data->maxy > SIZEOBSERVUNIV)
    {
        gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), NORMAL, "Isso é maior que o diâmetro do"
                            " Universo observável! Não exageremos..."                                );
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        data->maxy = SIZEOBSERVUNIV;
    }
    sprintf(t, "%11.3lE", data->minx);
    gtk_entry_set_text(GTK_ENTRY(data->minx_entry), t);
    sprintf(t, "%11.3lE", data->maxx);
    gtk_entry_set_text(GTK_ENTRY(data->maxx_entry), t);
    sprintf(t, "%11.3lE", data->miny);
    gtk_entry_set_text(GTK_ENTRY(data->miny_entry), t);
    sprintf(t, "%11.3lE", data->maxy);
    gtk_entry_set_text(GTK_ENTRY(data->maxy_entry), t);
}

void populate_combobox(pdata *data)
//Constrói a lista de nomes das cargas na combo box do editor de cargas.
{
    size_t i;
    char t[50];
    gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(data->ce_combobox));
    gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(data->ce_combobox), NULL, "");
    for (i = 0; i < data->numq; ++i)
    {
        sprintf(t, "%u: '%s'", i + 1, (data->qs[i]).name);
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(data->ce_combobox), NULL, t);
    }
}

gboolean save_sim(pdata *data, char *path)
//Guarda o estado da simulação.
{
    FILE *f;
    unsigned long long int count = 0;
    //Unsigned long long int para, em princípio, conter o size_t e mais ainda.
    char temp;
    f = fopen(path, "wb");
    if (!f)
    {
        return FALSE;
    }
    temp = data->state &(~ALWAYS) &(~JA);
    //Para não codificar aspectos da configuração...
    count += fwrite(&temp, sizeof(char), 1, f);
    count += fwrite(&data->numq, sizeof(size_t), 1, f);
    count += fwrite(data->qs, sizeof(charge), data->numq, f);
    count += fwrite(&data->numtq, sizeof(size_t), 1, f);
    count += fwrite(data->tqs, sizeof(testcharge), data->numtq, f);
    count += fwrite(&data->scale, sizeof(double), 1, f);
    count += fwrite(&data->timescale, sizeof(double), 1, f);
    count += fwrite(&data->minx, sizeof(double), 1, f);
    count += fwrite(&data->maxx, sizeof(double), 1, f);
    count += fwrite(&data->miny, sizeof(double), 1, f);
    count += fwrite(&data->maxy, sizeof(double), 1, f);
    count += fwrite(&data->offsetx, sizeof(double), 1, f);
    count += fwrite(&data->offsety, sizeof(double), 1, f);
    count += fwrite(&data->qedit, sizeof(size_t), 1, f);
    fclose(f);
    if (count != SIM_NUMELEM)
    {
        return FALSE;
    }
    return TRUE;
}

gboolean load_sim(pdata *data, char *path)
//Abre uma simulação guardada anteriormente.
{
    FILE *f;
    unsigned long long int count = 0;
    //Unsigned long long int para, em princípio, conter o size_t e mais ainda.
    char temp;
    double t2, t3;
    size_t t4;
    f = fopen(path, "rb");
    if (!f)
    {
        return FALSE;
    }
    count += fread(&temp, sizeof(char), 1, f);
    if (!check_debug(data->state) && check_debug(temp))
    {
        if(gtk_dialog_run(GTK_DIALOG(data->mbox1)) == GTK_RESPONSE_YES)
        {
            data->state |= temp;
        }
        else
        {
            data->state |= temp & UUUUUUUO;
        }
        gtk_widget_hide(data->mbox1);
    }
    else
    {
        data->state |= temp & UUUUUUUO;
    }
    switch(check_playmode(data->state))
    {
    case (ONLYTEST):
        gtk_range_set_value(GTK_RANGE(data->plmd_scale), 1);
        break;
    case (ALL):
        gtk_range_set_value(GTK_RANGE(data->plmd_scale), 2);
        break;
    default:
        if(check_debug(data->state))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "Há problemas na leitura"
                                " do modo de mexer as cargas..."                                    );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return FALSE;
        }
        break;
    }
    switch(check_drawmode(data->state))
    {
    case (VECTOR_DIRS):
        gtk_range_set_value(GTK_RANGE(data->vectmodes_scale), 1);
        break;
    case (FULL_VECTORS):
        gtk_range_set_value(GTK_RANGE(data->vectmodes_scale), 2);
    case (NONE):
    default:
        gtk_range_set_value(GTK_RANGE(data->vectmodes_scale), 0);
        break;
    }
    t4 = fread(&data->numq, sizeof(size_t), 1, f);
    if (!t4)
    {
        return FALSE;
    }
    count += t4;
    free(data->qs);
    data->qs = NULL;
    data->qs = (charge *) malloc(data->numq * sizeof(charge));
    if(!(data->qs) && data->numq)
    {
        gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), CRITICAL, "Erro na alocação de memória para as cargas!");
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        return FALSE;
    }
    count += fread(data->qs, sizeof(charge), data->numq, f);
    populate_combobox(data);
    t4 = fread(&data->numtq, sizeof(size_t), 1, f);
    if (!t4)
    {
        return FALSE;
    }
    count += t4;
    free(data->tqs);
    data->tqs = NULL;
    data->tqs = (testcharge *) malloc(data->numtq * sizeof(testcharge));
    if(!(data->tqs) && data->numtq)
    {
        gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), CRITICAL, "Erro na alocação de memória para as cargas de prova!");
        g_source_remove(data->to_clean);
        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        return FALSE;
    }
    count += fread(data->tqs, sizeof(testcharge), data->numtq, f);
    count += fread(&t2, sizeof(double), 1, f);
    count += fread(&data->timescale, sizeof(double), 1, f);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->tscale_spbutton), data->timescale);
    count += fread(&data->minx, sizeof(double), 1, f);
    count += fread(&data->maxx, sizeof(double), 1, f);
    count += fread(&data->miny, sizeof(double), 1, f);
    count += fread(&data->maxy, sizeof(double), 1, f);
    update_minmax_display(data);
    update_scale( t2, data, gtk_widget_get_allocated_width(data->drawarea)/2.0,
                  gtk_widget_get_allocated_height(data->drawarea)/2.0           );
    count += fread(&t2, sizeof(double), 1, f);
    count += fread(&t3, sizeof(double), 1, f);
    offset_change(t2, t3, data);
    count += fread(&t4, sizeof(size_t), 1, f);
    qedit_update(data, t4);
    fclose(f);
    if (count != SIM_NUMELEM)
    {
            return FALSE;
    }
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean save_config(pdata *data, char *path)
//Salva as configurações.
{
    FILE *f;
    unsigned char count;
    //Pelo número de elementos a ler, não precisamos de mais...
    char temp;
    f = fopen(path, "wb");
    if (!f)
    {
        return FALSE;
    }
    count += fwrite(&data->numvects, sizeof(unsigned char), 1, f);
    count += fwrite(&data->tqr, sizeof(double), 1, f);
    count += fwrite(&data->tqq, sizeof(double), 1, f);
    count += fwrite(&data->tqm, sizeof(double), 1, f);
    count += fwrite(&data->tqcolor, sizeof(GdkRGBA), 1, f);
    count += fwrite(&data->vectcolor, sizeof(GdkRGBA), 1, f);
    count += fwrite(&data->vectadjust, sizeof(double), 1, f);
    temp = check_savelast(data->state) | check_tqcoll(data->state);
    count += fwrite(&temp, sizeof(char), 1, f);
    fclose (f);
    if (count != CONFIG_NUMELEM)
    {
        return FALSE;
    }
    return TRUE;
}

gboolean load_config(pdata * data, char *path)
//Abre configurações guardadas.
{
    FILE *f;
    unsigned char count;
    //Pelo número de elementos a ler, não precisamos de mais...
    char temp;
    f = fopen(path, "rb");
    if (!f)
    {
        return FALSE;
    }
    count += fread(&data->numvects, sizeof(unsigned char), 1, f);
    count += fread(&data->tqr, sizeof(double), 1, f);
    count += fread(&data->tqq, sizeof(double), 1, f);
    count += fread(&data->tqm, sizeof(double), 1, f);
    count += fread(&data->tqcolor, sizeof(GdkRGBA), 1, f);
    count += fread(&data->vectcolor, sizeof(GdkRGBA), 1, f);
    count += fread(&data->vectadjust, sizeof(double), 1, f);
    count += fread(&temp, sizeof(char), 1, f);
    data->state |= temp;
    fclose (f);
    if (count != CONFIG_NUMELEM)
    {
        return FALSE;
    }
    return TRUE;

}

void reset_data(pdata *data, resetmode mode)
//Põe as informações pré-definidas, caso algo corra mal ou se o peça...
{
    if(mode & RES_SIM)
    {
        data->state &= UUUOOOOU;
        data->minx = -5.0;
        data->maxx = 5.0;
        data->miny = -5.0;
        data->maxy = 5.0;
        gtk_entry_set_text(GTK_ENTRY(data->minx_entry), "-5,000E+000");
        gtk_entry_set_text(GTK_ENTRY(data->miny_entry), "-5,000E+000");
        gtk_entry_set_text(GTK_ENTRY(data->maxx_entry), " 5,000E+000");
        gtk_entry_set_text(GTK_ENTRY(data->maxy_entry), " 5,000E+000");
        data->offsetx = -5.1;
        data->offsety = -5.1;
        data->scale = 0.01;
        update_scale( 0.01, data, gtk_widget_get_allocated_width(data->drawarea),
                      gtk_widget_get_allocated_height(data->drawarea)             );
        data->timescale = 0.00001;
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->tscale_spbutton), data->timescale);
        qedit_update(data, SIZE_MAX);
        data->state |= (VECTOR_DIRS | ONLYTEST);
        gtk_range_set_value(GTK_RANGE(data->plmd_scale), 1);
        free(data->tqs);
        data->tqs = NULL;
        data->numtq = 0;
        free(data->qs);
        data->qs = NULL;
        data->numq = 0;
        gtk_combo_box_text_remove_all(GTK_COMBO_BOX_TEXT(data->ce_combobox));
        gtk_combo_box_text_append(GTK_COMBO_BOX_TEXT(data->ce_combobox), NULL, "");
        create_charge(data, -4.0, -4.0, 0.1, 0.1, 0.1, (GdkRGBA) {1.0, 0.0, 0.0, 1.0}, "Cargazita exemplar");
    }
    if (mode & RES_CONFIG)
    {
        data->state &= OOOUUUUU;
        data->numvects = 10;
        data->tqr = 5.0;
        data->tqq = 1.0;
        data->tqm = 1.0;
        data->tqcolor = (GdkRGBA) {0.5, 0.5, 0.5, 0.5};
        data->vectcolor = (GdkRGBA) {0.0, 0.0, 0.75, 0.75};
        data->vectadjust = 10.0;
        data->state |= ALWAYS;
    }
    gtk_widget_queue_draw(data->drawarea);
}

void show_info (gboolean reuse, size_t num, gboolean is_test_charge, void *pointer, double pos_x, double pos_y)
//Mostra informações diversas na label debaixo da drawarea.
{
    char text[256];
    //Estimativa bastante por excesso do tamanho máximo do texto a imprimir...
    vect temp;
    static size_t i = 0;
    static gboolean is_tc = FALSE;
    static double x = 0.0, y = 0.0;
    static pdata* data = NULL;
    if (!reuse)
    {
        i = num;
        is_tc = is_test_charge;
        data = (pdata *) pointer;
        x = pos_x;
        y = pos_y;
    }
    if (data)
    {
        if (is_tc && i < data->numtq)
        {
            sprintf( text, "<small><tt><b>Carga de Prova</b> (%11.3lE m, %11.3lE m):\n"
                     "vx: %11.3lE m | vy: %11.3lE m\n</tt></small>",
                     (data->tqs[i]).x, (data->tqs[i]).y, (data->tqs[i]).vx, (data->tqs[i]).vy                      );
            gtk_label_set_markup(GTK_LABEL(data->i_label), text);
            return;
        }
        else if (i < data->numq)
        {
            sprintf( text, "<small><tt><b>%s</b> (%11.3lE m, %11.3lE m):\nvx: %11.3lE m/s | "
                     "vy: %11.3lE m/s\n(m: %10.3lE kg   | q: %11.3lE C)</tt></small>", (data->qs[i]).name,
                     (data->qs[i]).x, (data->qs[i]).y, (data->qs[i]).vx, (data->qs[i]).vy,
                     (data->qs[i]).m, (data->qs[i]).q                                                        );
            gtk_label_set_markup(GTK_LABEL(data->i_label), text);
            return;
        }
        else if (i == SIZE_MAX)
        {
            temp = calc_el(x, y, 1.0, data, TRUE);
            sprintf( text, "<small><tt>(%11.3lE m, %12.3lEm)\nE: %11.3lE N/C (%11.3lE ex |"
                     " %11.3lE ey)\n</tt></small>", x, y, temp.len, temp.x, temp.y          );
            gtk_label_set_markup(GTK_LABEL(data->i_label), text);
            return;
        }
    }
}

gboolean create_random_charge(double x, double y, double max_charge_limit, char name[32], pdata *data)
//Cria uma carga aleatória, mas tendo em conta limites e dimensões...
{
    max_charge_limit *= randouble(0.8, 7.5);
    double m_r = randouble(0.85*data->scale, 15.0*data->scale),
           q = randouble(ALMOSTZERO, max_charge_limit);
    if (x - m_r < data->minx)
    {
       x = data->minx + m_r;
    }
    else if (x + m_r > data->maxx)
    {
       x = data->maxx - m_r;
    }
    if (y - m_r < data->miny)
    {
       y = data->miny + m_r;
    }
    else if (y + m_r > data->maxy)
    {
       y = data->maxy - m_r;
    }
    if (!create_charge( data, x, y, m_r*randouble(0.8,1.25), m_r, randsignal() * q, (GdkRGBA)
                  {randouble(0.0, 1.0), randouble(0.0, 1.0), randouble(0.0, 1.0), 1.0}, name ) )
    {
        return FALSE;
    }
    q_coll_detect(data, data->numq - 1);
    return TRUE;
}

double si_conversion (char *s, siconvertmode mode, size_t len, char **rest)
//Lê a string s e reconhece (algumas) unidades e prefixos.
/* Convém ressalvar-se que, um pouco em contradição com as melhores
   práticas da programação, surgem aqui múltiplos "magic numbers",
   isto é, colocou-se os valores para a conversão directamente no
   código sem indicar propriamente o que significam, mas, pelo contexto,
   cremos que são suficientemente óbvios para isso não prejudicar
   a facilidade de leitura.                                              */
{
    size_t a;
    char *temp;
    if (len)
    {
        if (rest)
        {
            if (len > 1)
            {
                *rest = &s[1];
            }
            else
            {
                *rest = NULL;
            }
        }
        switch (mode)
        {
        case (PREFIXES):
            switch (s[0])
            {
            case 'E':
                return 1e18;
            case 'P':
                return 1e15;
            case 'T':
                return 1e12;
            case 'G':
                return 1e9;
            case 'M':
                return 1e6;
            case 'k':
                return 1e3;
            case 'h':
                return 1e2;
            case 'd':
                if (len > 1 && s[1] == 'a')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 10.0;
                }
                else
                {
                    return 0.1;
                }
                break;
            case 'c':
                return 1e-2;
            case 'm':
                return 1e-3;
            case BEGINMU1:
                if (len < 2 || s[1] != ENDMU1)
                {
                    break;
                }
                if (rest)
                {
                    if (len > 2)
                    {
                        *rest = &s[2];
                    }
                    else
                    {
                        *rest = NULL;
                    }
                }
                return 1e-6;
            case BEGINMU2:
                if (len < 2 || s[1] != ENDMU2)
                {
                    break;
                }
                if (rest)
                {
                    if (len > 2)
                    {
                        *rest = &s[2];
                    }
                    else
                    {
                        *rest = NULL;
                    }
                }
                return 1e-6;
            case 'u':
                return 1e-6;
            case 'n':
                return 1e-9;
            case 'p':
                return 1e-12;
            case 'f':
                return 1e-15;
            case 'a':
                return 1e-18;
            case 'z':
                return 1e-21;
            case 'y':
                return 1e-24;
            default:
                *rest = s;
                break;
            }
            break;
        case (LENGTH):
            switch(s[0])
            {
            case 'm':
                if (len == 1)
                {
                    break;
                }
                else if (len > 1 && s[1] == 'i')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 1609.344;
                }
                else
                {
                    return 1e-3 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case 'f':
                if (len == 1)
                {
                    *rest = s;
                    break;
                }
                else if (len > 1 && s[1] == 't')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 0.3048;
                }
                else
                {
                    return 1e-15 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case 'i':
                if (len > 1 && s[1] == 'n')
                {
                    return 0.0254;
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                }
                else
                {
                    *rest = s;
                    break;
                }
            case 'l':
                if (len > 1 && s[1] == 'y')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 9460730472580800.0;
                }
                else
                {
                    if (rest)
                    {
                        *rest = s;
                    }
                    break;
                }
            case 'a':
                if (len == 1)
                {
                    if (rest)
                    {
                        *rest = s;
                    }
                    break;
                }
                else if (len > 1 && s[1] == 'u')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 149597870700.0;
                }
                else
                {
                    return 1e-18 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case 'p':
                if (len == 1)
                {
                    if (rest)
                    {
                        *rest = s;
                    }
                    break;
                }
                else if (len > 1 && s[1] == 'c')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 3.085677581e16;
                }
                else
                {
                    return 1e-12 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case 'y':
                if (len == 1)
                {
                    *rest = s;
                    break;
                }
                else if (len > 1 && s[1] == 'd')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 0.9144;
                }
                else
                {
                    return 1e-24 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case BEGINANGS1:
                if (len < 2 || s[1] != ENDANGS1)
                {
                    break;
                }
                if (rest)
                {
                    if (len > 2)
                    {
                        *rest = &s[2];
                    }
                    else
                    {
                        *rest = NULL;
                    }
                }
                return 1e-10;
            case BEGINANGS2:
                if (len < 3 || s[1] != MIDDLEANGS2 || s[2] != ENDANGS2)
                {
                    break;
                }
                if (rest)
                {
                    if (len > 3)
                    {
                        *rest = &s[3];
                    }
                    else
                    {
                        *rest = NULL;
                    }
                }
                return 1e-10;
            default:
                if (rest)
                {
                    return  si_conversion(s, PREFIXES, len, rest) * si_conversion(*rest, LENGTH, strlen(*rest), NULL);
                }
                break;
            }
            break;
        case (TIME):
            switch(s[0])
            {
            case 's':
                break;
            case 'h':
                return 3600.0;
            case 'd':
                return 86400.0;
            case 'w':
                return 604800.0;
            case 'a':
                if (len == 1)
                {
                    return 1314000.0;
                }
                else
                {
                    return 1e-3 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            default:
                if (rest)
                {
                    return si_conversion(s, PREFIXES, len, rest) * si_conversion(*rest, TIME, strlen(*rest), NULL);
                }
                break;
            }
            break;
        case (MASS):
            switch(s[0])
            {
            case 'g':
                return 1e-3;
            case 't':
                return 1e3;
            case 'l':
                if (len > 1 && s[1] == 'b')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 0.45359237;
                }
                else
                {
                    *rest = s;
                    break;
                }
            case 'o':
                if (len > 1 && s[1] == 'z')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 2.8349523125e-3;
                }
                else
                {
                    if (rest)
                    {
                        *rest = s;
                    }
                    break;
                }
            case 's':
                if (len > 1 && s[1] == 't')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 6.35029318;
                }
                else if(len > 3 && s[1] == 'l' && s[2] == 'u' && s[3] == 'g')
                {
                    if (rest)
                    {
                        if (len > 4)
                        {
                            *rest = &s[4];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 14.59390294;
                }
                else
                {
                    if (rest)
                    {
                        *rest = s;
                    }
                    break;
                }
            default:
                if (rest)
                {
                    return si_conversion(s, PREFIXES, len, rest) * si_conversion(*rest, MASS, strlen(*rest), NULL);
                }
                break;
            }
            break;
        case (ELECTRIC_CHARGE):
            switch(s[0])
            {
            case 'C':
                break;
            case 'a':
                if (len == 1)
                {
                    *rest = s;
                    break;
                }
                else if (len > 2 && s[1] == 'b' && s[2] == 'C')
                {
                    if (rest)
                    {
                        if (len > 3)
                        {
                            *rest = &s[3];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 10.0;
                }
                else
                {
                    return 1e-18 * si_conversion(&s[1], LENGTH, len - 1, rest);
                }
            case 's':
                if (len > 4 && s[1] == 't' && s[2] == 'a' && s[3] == 't' && s[4] == 'C')
                {
                    if (rest)
                    {
                        if (len > 5)
                        {
                            *rest = &s[5];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return 3.335640951982e-10;
                }
                else
                {
                    *rest = s;
                    break;
                }
            case 'q':
                if (len > 1 && s[1] == 'e')
                {
                    if (rest)
                    {
                        if (len > 2)
                        {
                            *rest = &s[2];
                        }
                        else
                        {
                            *rest = NULL;
                        }
                    }
                    return qe;
                }
                else
                {
                    *rest = s;
                    break;
                }
            break;
            default:
                if (rest)
                {
                    return si_conversion(s, PREFIXES, len, rest) * si_conversion(*rest, ELECTRIC_CHARGE, strlen(*rest), NULL);
                }
                break;
            }
        case (VELOCITY):
            if (rest)
            {
                for (a = 0; s[a] != 0 && a < len; ++a)
                {
                    if (s[a] == '/' || s[a] == '\\' || s[a] == '|')
                    {
                        return si_conversion(s, LENGTH, a, &temp)/si_conversion(&s[a+1], TIME, len - a - 1, rest);
                    }
                }
                break;
            }
            break;
        default:
            break;
        }
    }
    return 1.0;
}

/* ---------------------------------------------------------------- */
/* --------------------- CALLBACKS e TIMEOUTS --------------------- */
/* ---------------------------------------------------------------- */

gboolean cleanstatusbar (GtkWidget *sb)
//Limpa a status bar, de tempos a tempos.
{
    int i;
    for (i=0; i < SBC_LAST; ++i)
    {
        gtk_statusbar_pop(GTK_STATUSBAR(sb), i);
    }
    return TRUE;
}

/* ********************** */
/*  O CALLBACK DO TEMPO!  */
/* ********************** */

gboolean timeout (pdata *data)
//Faz todos os cálculos do movimento das coisas, e outras actualizações periódicas de informação.
{
    size_t i;
    vect a;
    int b, c;
    show_info(TRUE, 0, FALSE, data, 0.0, 0.0);
    b = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->rightarrow)) -
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->leftarrow))    ;
    c = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->downarrow)) -
        gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->uparrow))      ;
    if (b || c)
    {
        offset_change(data->offsetx + b * 2.0 * data->scale, data->offsety + c * 2.0 * data->scale, data);
        gtk_widget_queue_draw(data->drawarea);
    }
    if (data->qedit < data->numq)
    {
        qedit_update(data, data->qedit);
        gtk_widget_queue_draw(data->drawarea);
    }
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->playbutton)))
    {
        switch(check_playmode(data->state))
        {
        case (ALL):
            if (data->numq)
            {
                for (i=0; i < data->numq; ++i)
                {
                    a = calc_el((data->qs[i]).x, (data->qs[i]).y, (data->qs[i]).q, data, FALSE);
                    (data->qs[i]).vx += TIMEINTERVAL*(data->timescale)*(a.x)/((data->qs[i]).m);
                    (data->qs[i]).vy += TIMEINTERVAL*(data->timescale)*(a.y)/((data->qs[i]).m);
                }
                for (i=0; i < data->numq; ++i)
                {
                    (data->qs[i]).x += TIMEINTERVAL*((data->qs[i]).vx)*(data->timescale);
                    (data->qs[i]).y += TIMEINTERVAL*((data->qs[i]).vy)*(data->timescale);
                    q_coll_detect(data, i);
                }
            }
        case (ONLYTEST):
            if (data->numtq)
            {
                for (i=0; i < data->numtq; ++i)
                {
                    a = calc_el((data->tqs[i]).x, (data->tqs[i]).y, data->tqq, data, FALSE);
                    (data->tqs[i]).vx += TIMEINTERVAL*(a.x)/(data->tqm)*(data->timescale);
                    (data->tqs[i]).vy += TIMEINTERVAL*(a.y)/(data->tqm)*(data->timescale);
                }
                for (i=0; i < data->numtq; ++i)
                {
                    (data->tqs[i]).x += TIMEINTERVAL*((data->tqs[i]).vx)*(data->timescale);
                    (data->tqs[i]).y += TIMEINTERVAL*((data->tqs[i]).vy)*(data->timescale);
                    if(check_tqcoll(data->state))
                    {
                        tq_coll_detect(data, i);
                    }
                }
            }
        default:
            break;
        }
        gtk_widget_queue_draw(data->drawarea);
    }
    return TRUE;
}

/* ********************** */
/*  O CALLBACK DE SCROLL  */
/* ********************** */

gboolean scroll (GtkWidget *widget, GdkEvent *event, gpointer p)
//Lida com o zoom pelo rato.
{

    pdata *data = (pdata *) p;
    double old_scale = data->scale;
    switch(event->scroll.direction)
    {
    case(GDK_SCROLL_UP):
        update_scale(data->scale * 0.8, data, event->scroll.x, event->scroll.y);
        break;
    case(GDK_SCROLL_DOWN):
        update_scale(data->scale * 1.25, data, event->scroll.x, event->scroll.y);
        break;
    default:
        break;
    }
    gtk_widget_queue_draw(data->drawarea);
    return FALSE;
}

/* ********************* */
/*  O CALLBACK DO RATO!  */
/* ********************* */

gboolean mouse_pan (GtkWidget *widget, GdkEvent *event, gpointer *p)
//Lida com as acções do rato na drawing area.
{
    pdata *data = (pdata *) p;
    char t[25];
    static double tx=0.0, ty=0.0, dex = 0.0, dey = 0.0;
    //dex e dey representam o desvio do ponteiro do rato do centro da carga mexida.

    static size_t i = SIZE_MAX;

    if (event->type == GDK_2BUTTON_PRESS && event->button.button == 1)
    {
        for (i=0; i < data->numq ; ++i)
        {
            if ( ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) *
                 ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) +
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) *
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) <
                 ((data->qs[i]).r) * ((data->qs[i]).r)                          )

            /* A despeito das questões das condições nos ciclos, aqui é só um if,
               pelo que não impacta negativamente o desempenho...                 */

            {
                qedit_update(data, i);
                i = SIZE_MAX;
                return TRUE;
            }
        }
        create_test_charge( event->button.x*data->scale + data->offsetx,
                            event->button.y*data->scale + data->offsety, data   );
        qedit_update(data, SIZE_MAX);
        i = SIZE_MAX;
        return TRUE;
    }
    else if (event->type == GDK_2BUTTON_PRESS && event->button.button == 3)
    {
        tx = 2.0*ALMOSTZERO; //Valor mínimo para a carga máxima.
        for (i=0; i < data->numq ; ++i)
        {
            if ((data->qs[i]).q > tx)
            {
                tx = (data->qs[i]).q;
            }
            if ( ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) *
                 ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) +
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) *
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) <
                 ((data->qs[i]).r) * ((data->qs[i]).r)                          )

            /* A despeito das questões das condições nos ciclos, aqui é só um if,
               pelo que não impacta negativamente o desempenho...                 */

            {
                destroy_charge(i, data);
                i = SIZE_MAX;
                tx = 0.0;
                return TRUE;
            }
        }
        create_random_charge( event->button.x*data->scale + data->offsetx,
                              event->button.y*data->scale + data->offsety,
                              tx, "CARGA ALEATÓRIA", data                  );
        i = SIZE_MAX;
        tx = 0.0;
        return TRUE;
    }
    else if (event->type == GDK_BUTTON_PRESS && event->button.button == 1)
    {
        for (i=0; i < data->numq ; ++i)
        {
            dex = event->button.x*data->scale + data->offsetx - (data->qs[i]).x;
            dey = event->button.y*data->scale + data->offsety - (data->qs[i]).y;
            if (dex * dex + dey * dey < (data->qs[i]).r * (data->qs[i]).r)
            {
                return TRUE;
            }
        }
        tx = event->button.x*data->scale;
        ty = event->button.y*data->scale;
        i = SIZE_MAX;
        /* A seguir, vai-se atribuir os valores
           "infinito" e "NaN" a dex e dey caso
           o ponteiro esteja próximo dos limites
           da área, de modo a que se os possa
           arrastar para expandir...             */
        dex = 0.0;
        dey = 0.0;
        if (fabs(tx + data->offsetx - data->minx) < data->scale * 2.0)
        {
            dex = NAN;
        }
        else if (fabs(tx + data->offsetx - data->maxx) < data->scale * 2.0)
        {
            dex = INFINITY;
        }
        if (fabs (ty + data->offsety - data->miny) < data->scale * 2.0)
        {
            dey = NAN;
        }
        else if (fabs (ty + data->offsety - data->maxy) < data->scale * 2.0)
        {
            dey = INFINITY;
        }
        return TRUE;
    }
    else if ((tx > ALMOSTZERO || ty > ALMOSTZERO) && event->type == GDK_MOTION_NOTIFY)
    {
        if (isfinite(dex) && isfinite(dey))
        {
            offset_change( data->offsetx - event->motion.x*data->scale + tx,
                           data->offsety - event->motion.y*data->scale + ty,
                           data                                              );
        }
        else
        {
            if (isnan(dex))
            {
                data->minx += event->motion.x*data->scale - tx;
                if (data->minx >= data->maxx)
                {
                    data->minx = data->maxx;
                }
            }
            else if (isinf(dex))
            {
                data->maxx += event->motion.x*data->scale - tx;
                if (data->maxx <= data->minx)
                {
                    data->maxx = data->minx;
                }
            }
            if (isnan(dey))
            {
                data->miny += event->motion.y*data->scale - ty;
                if (data->miny >= data->maxy)
                {
                    data->miny = data->maxy;
                }
            }
            else if (isinf(dey))
            {
                data->maxy += event->motion.y*data->scale - ty;
                if (data->maxy < data->miny)
                {
                    data->maxy = data->miny;
                }
            }
            update_minmax_display(data);
        }
        tx = event->button.x*data->scale;
        ty = event->button.y*data->scale;
        gtk_widget_queue_draw(data->drawarea);
        return TRUE;
    }
    else if (i < data->numq && event->type == GDK_MOTION_NOTIFY)
    {
        tx = event->motion.x*data->scale + data->offsetx - dex;
        ty = event->motion.y*data->scale + data->offsety - dey;
        (data->qs[i]).x = (tx > data->minx + (data->qs[i]).r ?
                          (tx < data->maxx - (data->qs[i]).r ?
                          tx : data->maxx - (data->qs[i]).r) :
                          data->minx + (data->qs[i]).r);

        (data->qs[i]).y = (ty > data->miny + (data->qs[i]).r ?
                          (ty < data->maxy - (data->qs[i]).r ?
                          ty : data->maxy - (data->qs[i]).r) :
                           data->miny + (data->qs[i]).r);

        if (!(event->motion.state & GDK_CONTROL_MASK))
        {
            (data->qs[i]).vx = 0.0;
            (data->qs[i]).vy = 0.0;
        }
        q_coll_detect(data, i);
        gtk_widget_queue_draw(data->drawarea);
        tx = 0.0;
        ty = 0.0;
        return TRUE;
    }
    else if (event->type == GDK_MOTION_NOTIFY)
    {
        for (i = 0; i < data->numq ; ++i)
        {
            if ( ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) *
                 ((event->button.x*data->scale + data->offsetx) - ((data->qs[i]).x)) +
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) *
                 ((event->button.y*data->scale + data->offsety) - ((data->qs[i]).y)) <
                 ((data->qs[i]).r) * ((data->qs[i]).r)                          )

            {
                show_info(FALSE, i, FALSE, data, 0.0, 0.0);
                i = SIZE_MAX;
                return TRUE;
            }
        }
        for (i = 0; i < data->numtq ; ++i)
        {
            if ( ((event->button.x*data->scale + data->offsetx) - ((data->tqs[i]).x)) *
                 ((event->button.x*data->scale + data->offsetx) - ((data->tqs[i]).x)) +
                 ((event->button.y*data->scale + data->offsety) - ((data->tqs[i]).y)) *
                 ((event->button.y*data->scale + data->offsety) - ((data->tqs[i]).y)) <
                 data->tqr * data->tqr * data->scale * data->scale                       )

            {
                show_info(FALSE, i, TRUE, data, 0.0, 0.0);
                i = SIZE_MAX;
                return TRUE;
            }
        }
        i = SIZE_MAX;
        show_info( FALSE, i, FALSE, data, event->button.x*data->scale + data->offsetx,
                   event->button.y*data->scale + data->offsety                         );

    }
    else if (event->type == GDK_BUTTON_RELEASE && event->button.button == 1)
    {
        tx = 0.0;
        ty = 0.0;
        i = SIZE_MAX;
        return TRUE;
    }
    return TRUE;
}

/* ************************ */
/*  O CALLBACK DE DESENHO!  */
/* ************************ */

gboolean draw_stuff (GtkWidget *w, cairo_t *cr, gpointer p)
//Desenha as coisinhas todas.
{
    pdata *data = (pdata *) p;
    size_t i;
    vect temp;
    double maxq = ALMOSTZERO, vectlencorr, tx, ty,
           realwidth = min(data->maxx - data->minx, data->scale * gtk_widget_get_allocated_width(data->drawarea)),
           realheight = min(data->maxy - data->miny, data->scale * gtk_widget_get_allocated_height(data->drawarea)),
           deltax = realwidth/((double) data->numvects + 1.0),
           deltay = realheight/((double) data->numvects + 1.0),
           extremx = min(data->scale * gtk_widget_get_allocated_width(data->drawarea)
                     + data->offsetx, data->maxx),
           extremy = min(data->scale * gtk_widget_get_allocated_height(data->drawarea)
                     + data->offsety, data->maxy);

    cairo_scale(cr, 1/(data->scale), 1/(data->scale));
    //Maior escala implica desenho mais pequeno.

    cairo_translate(cr, -data->offsetx, -data->offsety);
    //Um offset positivo aproxima as coisas da origem...

    cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
    cairo_set_line_width(cr, 2.0*data->scale);
    cairo_rectangle(cr, data->minx, data->miny, data->maxx - data->minx, data->maxy - data->miny);
    cairo_stroke(cr);
    cairo_set_line_width(cr, data->scale);
    if (data->numtq)
    {
        vectlencorr = data->tqr * data->scale; //Temporário.
        for(i=0; i < data->numtq; ++i)
        {
            if ( tq_edge_detect( data, i, simple_max(data->offsetx, data->minx) - vectlencorr,
                                 simple_max(data->offsety, data->miny) - vectlencorr, extremx +
                                 vectlencorr, extremy + vectlencorr                              ) )
            {
                --i;
                continue;
            }
            cairo_arc(cr,(data->tqs[i]).x, (data->tqs[i]).y, (data->tqr)*(data->scale), 0, 2*M_PI);
            gdk_cairo_set_source_rgba(cr, &(data->tqcolor));
            cairo_fill_preserve(cr);
            cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
            cairo_stroke(cr);
        }
    }
    if (data->numq)
    {
        for (i=0; i < data->numq; ++i)
        {
            if(fabs((data->qs[i]).q) > maxq)
            {
                maxq = fabs((data->qs[i]).q);
            }
            gdk_cairo_set_source_rgba(cr, &((data->qs[i]).color));
            if (i == data->qedit && (time(NULL) & 1))
            {
                cairo_arc(cr,(data->qs[i]).x, (data->qs[i]).y, (data->qs[i]).r, 0, 2*M_PI);
                cairo_fill_preserve(cr);
                cairo_set_source_rgba(cr, 1.0, 1.0, 0.0, 1.0);
                cairo_set_line_width(cr, 5*data->scale);
                cairo_stroke(cr);
                cairo_set_line_width(cr, data->scale);
            }
            else
            {
                cairo_arc(cr,(data->qs[i]).x, (data->qs[i]).y, (data->qs[i]).r, 0, 2*M_PI);
                cairo_fill_preserve(cr);
                cairo_set_source_rgba(cr, 0.0, 0.0, 0.0, 1.0);
                cairo_stroke(cr);
            }
        }
        gdk_cairo_set_source_rgba(cr, &(data->vectcolor));
        switch(check_drawmode(data->state))
        {
        case (FULL_VECTORS):
            vectlencorr = (data->vectadjust)*(data->scale)/(maxq*K);
            for ( ty = (data->miny > data->offsety ? data->miny :
                  floor((data->offsety - data->miny)/deltay) * deltay +
                  data->miny) + deltay; ty < extremy; ty += deltay      )
            {
                for ( tx = (data->minx > data->offsetx ? data->minx :
                      floor((data->offsetx - data->minx)/deltax) * deltax +
                      data->minx) + deltax; tx < extremx; tx += deltax      )
                {
                    temp = calc_el(tx, ty, 1.0, data, TRUE);
                    if (!(temp.x > ALMOSTZERO || temp.y > ALMOSTZERO || temp.len > ALMOSTZERO))
                    {
                        continue;
                    }
                    vect_to_size(&temp, vectlencorr * (temp.len));
                    if ( tx + temp.x < data->minx || tx + temp.x > data->maxx ||
                         ty + temp.y < data->miny || ty + temp.y > data->maxy    )
                    {
                        continue;
                    }
                    draw_vect(&temp, tx, ty, cr);
                }
            }
            break;
        case (VECTOR_DIRS):
            vectlencorr = data->vectadjust * data->scale;
            for ( ty = (data->miny > data->offsety ? data->miny :
                  floor((data->offsety - data->miny)/deltay) * deltay +
                  data->miny) + deltay; ty < extremy; ty += deltay      )
            {
                for ( tx = (data->minx > data->offsetx ? data->minx :
                      floor((data->offsetx - data->minx)/deltax) * deltax +
                      data->minx) + deltax; tx < extremx; tx += deltax      )
                {
                    temp = calc_el(tx, ty, 1.0, data, TRUE);
                    if (!(temp.x > ALMOSTZERO || temp.y > ALMOSTZERO || temp.len > ALMOSTZERO))
                    {
                        continue;
                    }
                    vect_to_size(&temp, vectlencorr);
                    if ( tx + temp.x < data->minx || tx + temp.x > data->maxx ||
                         ty + temp.y < data->miny || ty + temp.y > data->maxy    )
                    {
                        continue;
                    }
                    draw_vect(&temp, tx, ty, cr);
                }
            }
            break;
        default:
            break;
        }
    }
    return FALSE;
}

gboolean quit(GtkWidget *w, gpointer p)
//Callback para guardar o último estado, desligar os timeouts e sair do programa.
{
    if (!p)
    {
        gtk_main_quit();
        return TRUE;
    }
    pdata *data = (pdata *) p;
    if (check_savelast(data->state))
    {
        save_sim(data, LASTSTATE);
    }
    save_config(data, CONFIG);
    g_source_remove(data->to_motion);
    g_source_remove(data->to_clean);
    gtk_main_quit();
    return TRUE;
}

gboolean v_scale_change(GtkRange *r, gpointer p)
//Callback da scale da escala de visualização.
{
    pdata *data = (pdata *) p;
    double t = gtk_range_get_value(r) * data->scaleadjvalue;
    if (fabs(t - data->scale) > DBL_EPSILON * data->scaleadjvalue)
    {
        update_scale( t, data, gtk_widget_get_allocated_width(data->drawarea)/2.0,
                      gtk_widget_get_allocated_height(data->drawarea)/2.0          );
        gtk_widget_queue_draw(data->drawarea);
    }
    return TRUE;
}

gboolean reset_button (GtkWidget *w, gpointer p)
//Callback do botão de reset.
{
  reset_data((pdata *)  p, RES_SIM);
  return TRUE;
}

gboolean show_help(GtkWidget *w, gpointer p)
//Callback do botão de mostrar a ajuda.
{
    pdata *data = (pdata *) p;
    gtk_widget_show_all(data->mbox2);
    return TRUE;
}

gboolean show_options(GtkWidget *w, gpointer p)
//Callback do botão de mostrar as opções.
{
    char t[25];
    pdata *data = (pdata *) p;
    sprintf(t,"%11.3lE", data->tqq);
    gtk_entry_set_text(GTK_ENTRY(data->tqq_entry), t);
    sprintf(t,"%11.3lE", data->tqr);
    gtk_entry_set_text(GTK_ENTRY(data->tqr_entry), t);
    sprintf(t,"%11.3lE", data->tqm);
    gtk_entry_set_text(GTK_ENTRY(data->tqm_entry), t);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->colorbtq), &data->tqcolor);
    gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->colorbvect), &data->vectcolor);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->numvects_sb), (double) data->numvects);
    gtk_range_set_value(GTK_RANGE(data->vadj_scale), data->vectadjust);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->tqc_b), (check_tqcoll(data->state) ? 1 : 0));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->sl_b), (check_savelast(data->state) ? 1 : 0));
    gtk_widget_show_all(data->optiondialog);
    return TRUE;
}

gboolean select_vector_color(GtkWidget *w, gpointer p)
//Callback do botão de escolher a cor dos vectores.
{
  pdata *data = (pdata *) p;
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->colorbvect), &data->vectcolor);
  gtk_widget_queue_draw(data->drawarea);
  return TRUE;
}

gboolean select_test_charge_color(GtkWidget *w, gpointer p)
//Callback do botão de escolher a cor das cargas de prova.
{
  pdata *data = (pdata *) p;
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(data->colorbtq), &data->tqcolor);
  gtk_widget_queue_draw(data->drawarea);
  return TRUE;
}

gboolean play_mode_change(GtkWidget *w, gpointer p)
//Callback da scale dos modos de representação.
{
    pdata *data = (pdata *) p;
    char a = ((char) round(gtk_range_get_value(GTK_RANGE(data->plmd_scale)))) * ONLYTEST;
    if (a != check_playmode(data->state))
    {
        data->state &= (~plmd_cmp);
        data->state |= a;
    }
    return TRUE;
}

gboolean change_timescale(GtkWidget *w, gpointer p)
//Callback do spin button da escala de tempo.
{
    pdata *data = (pdata *) p;
    double t = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->tscale_spbutton));
    if (fabs(t - data->timescale) > DBL_EPSILON)
    {
        data->timescale = t;
    }
    return TRUE;
}

gboolean save_button (GtkWidget *w, gpointer p)
//Callback do botão "guardar".
{
    pdata *data = (pdata *) p;
    size_t a;
    gchar *temp;
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(data->f_saver), ".");
    if (gtk_dialog_run(GTK_DIALOG(data->f_saver)) != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_hide(data->f_saver);
        return TRUE;
    }
    temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(data->f_saver));
    a = strlen(temp);
    if (temp[a-4] != '.' || temp[a-3] != 's' || temp[a-2] != 'c' || temp[a-1] != 'e')
    {
        temp = (gchar *) realloc(temp, a + 5);
        temp[a] = '.';
        ++a;
        temp[a] = 's';
        ++a;
        temp[a] = 'c';
        ++a;
        temp[a] = 'e';
        ++a;
        temp[a] = 0;
    }
    if (!save_sim(data, temp))
    {
            gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS,
                                "Não foi possível guardar a simulação escolhida..." );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
    }
    g_free(temp);
    gtk_widget_hide(data->f_saver);
    return TRUE;
}

gboolean load_button (GtkWidget *w, gpointer p)
//Callback do botão "abrir".
{
    pdata *data = (pdata *) p;
    gchar *temp;
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(data->f_loader), ".");
    if (gtk_dialog_run(GTK_DIALOG(data->f_loader)) != GTK_RESPONSE_ACCEPT)
    {
        gtk_widget_hide(data->f_loader);
        return TRUE;
    }
    temp = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(data->f_loader));
    if (!load_sim(data, temp))
    {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS,
                                "Não foi possível abrir a simulação escolhida."
                                " Adoptando, para garantir a continuidade do uso,"
                                " valores pré-definidos...");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            reset_data(data, RES_SIM);
    }
    g_free(temp);
    gtk_widget_hide(data->f_loader);
    return TRUE;
}

/* Estas três funções aglomeram as entradas para eliminar a necessidade
   de haver vários callbacks; originalmente, isto era efectuado por
   intermédio de funções genéricas number_entry e non_negative_number_entry,
   mas, dado que se tornava necessário emitir mensagens de erro na statusbar,
   optou-se por fazer assim...                                               */

gboolean offset_entries (GtkWidget *w, gpointer p)
//Callback das entries do offset.
{
    pdata *data = (pdata *) p;
    double t;
    if (w == data->offx_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->offsetx = t;
    }
    else if (w == data->offy_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->offsety = t;
    }
    else
    {
        if(check_debug(data->state))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "Há um erro na associação das"
                                                                         " entradas dos offsets..."     );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
    }
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean tq_properties_entries (GtkWidget *w, gpointer p)
//Callback das entradas das propriedadeas das cargas de prova.
{
    pdata *data = (pdata *) p;
    double t;
    if (w == data->tqm_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t) || t < ALMOSTZERO)
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO! (Deverá ser superior a 1e-100)");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->tqm = t;
    }
    else if (w == data->tqr_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t) || t < ALMOSTZERO)
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO! (Deverá ser superior a 1e-100)");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->tqr = t;
    }
    else if (w == data->tqq_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->tqq = t;
    }
    else
    {
        if(check_debug(data->state))
        {
            gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS, "Há um erro na associação"
                                "das entradas das propriedades das cargas..."                        );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
    }
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean minmax_entries (GtkWidget *w, gpointer p)
//Callback das entries para x e y mínimos e máximos.
{
    pdata *data = (pdata *) p;
    double t;
    if (w == data->minx_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->minx = ( t > data->maxx ? data->maxx : t);
    }
    else if (w == data->miny_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->miny = ( t > data->maxy ? data->maxy : t);
    }
    else if (w == data->maxx_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->maxx = ( t < data->minx ? data->minx : t);
    }
    else if (w == data->maxy_entry)
    {
        if (!sscanf(gtk_entry_get_text(GTK_ENTRY(w)), "%lf", &t))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
        }
        data->maxy = ( t < data->minx ? data->minx : t);
    }
    else
    {
        if(check_debug(data->state))
        {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "Há um erro na associação das entradas dos mínimos e máximos...");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
    }
    update_minmax_display(data);
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean hide (GtkWidget *w, gint r, gpointer p)
//Callback para esconder widgets (nomeadamente o diálogo de ajuda).
{
    gtk_widget_hide(w);
    return TRUE;
}

gboolean option_response (GtkWidget *w, gint r, gpointer p)
//Lida com as respostas do diálogo de opções.
{
    char t[25];
    pdata *data = (pdata *) p;
    if (r == GTK_RESPONSE_HELP)
    {
        reset_data(data, RES_CONFIG);
        sprintf(t,"%11.3lE", data->tqq);
        gtk_entry_set_text(GTK_ENTRY(data->tqq_entry), t);
        sprintf(t,"%11.3lE", data->tqr);
        gtk_entry_set_text(GTK_ENTRY(data->tqr_entry), t);
        sprintf(t,"%11.3lE", data->tqm);
        gtk_entry_set_text(GTK_ENTRY(data->tqm_entry), t);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->colorbtq), &data->tqcolor);
        gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(data->colorbvect), &data->vectcolor);
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->numvects_sb), (double) data->numvects);
        gtk_range_set_value(GTK_RANGE(data->vadj_scale), data->vectadjust);
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->tqc_b), (check_tqcoll(data->state) ? 1 : 0));
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->sl_b), (check_savelast(data->state) ? 1 : 0));
        return TRUE;
    }
    gtk_widget_hide(w);
    return TRUE;
}

gboolean change_numvects (GtkWidget *w, gpointer p)
//Callback do spin button do número de vectores.
{
    pdata *data = (pdata *) p;
    data->numvects = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->numvects_sb));
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean vectadjust_change (GtkWidget *w, gpointer p)
//Callback da range do ajuste do tamanho dos vectores.
{
    pdata *data = (pdata *) p;
    data->vectadjust = gtk_range_get_value(GTK_RANGE(data->vadj_scale));
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean vector_mode_change (GtkWidget *w, gpointer p)
//Callback da range do modo de representação dos vectores.
{
    pdata *data = (pdata *) p;
    data->state &= ~(drmd_cmp);
    data->state |= VECTOR_DIRS*((int) round(gtk_range_get_value(GTK_RANGE(data->vectmodes_scale))));
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;
}

gboolean tqc_toggle (GtkWidget *w, gpointer p)
//Callback do check button de ter cargas de prova etéreas.
{
    pdata *data = (pdata *) p;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->tqc_b)))
    {
        data->state |= JA;
    }
    else
    {
        data->state &= ~JA;
    }
    gtk_widget_queue_draw(data->drawarea);
    return TRUE;

}

gboolean sl_toggle (GtkWidget *w, gpointer p)
//Callback do check button de guardar o último estado.
{
    pdata *data = (pdata *) p;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->sl_b)))
    {
        data->state |= ALWAYS;
    }
    else
    {
        data->state &= ALWAYS;
    }
    return TRUE;

}

gboolean add_random_charges (GtkWidget *w, gpointer p)
//Callback da entry de cargas aleatórias.
{
    pdata *data = (pdata *) p;
    char *throwaway;
    unsigned long long num = strtoull(gtk_entry_get_text(GTK_ENTRY(data->addq_entry)), &throwaway, 10);
    if (!num || num > SIZE_MAX)
    {
            gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
            return TRUE;
    }
   while(num)
    {
        if (!create_random_charge( randouble(data->minx, data->maxx), randouble(data->miny, data->maxy),
                                   randouble(0.001, 5.0), "CARGA ALEATÓRIA", data)                        )
        {
            break;
        }
        --num;
    }
    gtk_entry_set_text(GTK_ENTRY(data->addq_entry), "");
    return TRUE;
}

gboolean change_qedit (GtkWidget *w, gpointer p)
{
    pdata *data = (pdata *) p;
    gint elem = gtk_combo_box_get_active(GTK_COMBO_BOX(data->ce_combobox)) - 1;
    if (elem < 0)
    {
        qedit_update(data, SIZE_MAX);
        return TRUE;
    }
    else if (elem > data->numq && elem != SIZE_MAX)
    {
        if (check_debug(data->state))
        {
            gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS, "Escolheu-se uma carga"
                                " aparentemente inexistente..."                                   );
            g_source_remove(data->to_clean);
            data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
        return TRUE;
    }
    if (elem != data->qedit)
    {
        qedit_update(data, (size_t) elem);
    }
    return TRUE;
}

gboolean chargeditorinput (GtkWidget *w, gpointer p)
//Callback de todos os widgets do editor de cargas.
{
    pdata *data = (pdata *) p;
    double num;
    char *t, *u, v[50];
    size_t i = 0, j = 0;
    if (w == data->ce_colorbutton)
    {
        gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(w), &data->qs[data->qedit].color);
        return TRUE;
    }
    else if (w == data->ce_delchargbutton)
    {
        destroy_charge(data->qedit, data);
    }
    else
    {
        t = gtk_entry_get_text(GTK_ENTRY(w));
        if (w == data->ce_namentry)
        {
            i = data->qedit + 1;
            strcpy(data->qs[data->qedit].name, t);
            sprintf(v, "%u: '%s'", i, t);
            gtk_combo_box_text_remove(GTK_COMBO_BOX_TEXT(data->ce_combobox), i);
            gtk_combo_box_text_insert(GTK_COMBO_BOX_TEXT(data->ce_combobox), i, NULL, v);
            gtk_combo_box_set_active(GTK_COMBO_BOX(data->ce_combobox), i);
        }
        else
        {
            num = strtod(t, &u);
            t = NULL;
            if (u)
            {
                for (i = 0; u[i] != 0; ++i)
                {
                    if (iscntrl(u[i]) || ispunct(u[i]) || isspace(u[i]))
                    {
                        continue;
                    }
                    if (!t)
                    {
                        j = i;
                        t = &u[i];
                    }
                }
                i -= j;
            }
            if (w == data->ce_mentry)
            {
                if (num < ALMOSTZERO)
                {
                    gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
                    g_source_remove(data->to_clean);
                    data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
                    return TRUE;
                }
                data->qs[data->qedit].m = num * si_conversion(t, MASS, i, &u);
                return TRUE;
            }
            else if (w == data->ce_qentry)
            {
                data->qs[data->qedit].q = num * si_conversion(t, ELECTRIC_CHARGE, i, &u);
            }
            else if (w == data->ce_rentry)
            {
                if (num < ALMOSTZERO)
                {
                    gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "NÚMERO INVÁLIDO!");
                    g_source_remove(data->to_clean);
                    data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
                    return TRUE;
                }
                data->qs[data->qedit].r = num * si_conversion(t, LENGTH, i, &u);
            }
            else if (w == data->ce_vxentry)
            {
                data->qs[data->qedit].vx = num * si_conversion(t, VELOCITY, i, &u);
            }
            else if (w == data->ce_vyentry)
            {
                data->qs[data->qedit].vy = num * si_conversion(t, VELOCITY, i, &u);
            }
            else if (w == data->ce_xentry)
            {
                data->qs[data->qedit].x = num * si_conversion(t, LENGTH, i, &u);
                if (data->qs[data->qedit].x + data->qs[data->qedit].r > data->maxx)
                {
                    data->qs[data->qedit].x = data->maxx - data->qs[data->qedit].r;
                }
                else if (data->qs[data->qedit].x - data->qs[data->qedit].r < data->minx)
                {
                    data->qs[data->qedit].x = data->minx + data->qs[data->qedit].r;
                }
            }
            else if (w == data->ce_yentry)
            {
                data->qs[data->qedit].y = num * si_conversion(t, LENGTH, i, &u);
                if (data->qs[data->qedit].y + data->qs[data->qedit].r > data->maxy)
                {
                    data->qs[data->qedit].y = data->maxy - data->qs[data->qedit].r;
                }
                else if (data->qs[data->qedit].y - data->qs[data->qedit].r < data->miny)
                {
                    data->qs[data->qedit].y = data->miny + data->qs[data->qedit].r;
                }
            }
            else
            {
                if(check_debug(data->state))
                {
                    gtk_statusbar_push(GTK_STATUSBAR(data->statusbar), WARNINGS, "Há um erro na associação das entradas da edição de cargas...");
                    g_source_remove(data->to_clean);
                    data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
                }
            }
        }
    }
    gtk_widget_queue_draw(data->drawarea);
    show_info(TRUE, 0, FALSE, NULL, 0.0, 0.0);
    return TRUE;
}

/* ---------------------------------------------------------------- */
/* ---------------------- INICIALIZAÇÕES  ------------------------- */
/* ---------------------------------------------------------------- */


void init_widgets (pdata *data)
//Inicializa toda a questão dos widgets.
{
        GError dummy, *p = &dummy; //Só para o GTK não reclamar se o ícone estiver ausente...
        GtkFileFilter *sf, *af;
        GtkWidget *dcontent;

        data->mainwindow = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_default_size(GTK_WINDOW(data->mainwindow), 800, 720);
        gtk_window_set_title(GTK_WINDOW(data->mainwindow), "Simulador de Campos Eléctricos");
        gtk_window_set_position(GTK_WINDOW(data->mainwindow), GTK_WIN_POS_CENTER);
        gtk_window_set_icon_from_file(GTK_WINDOW(data->mainwindow), ICON, &p);

        data->mainvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

        gtk_container_add(GTK_CONTAINER(data->mainwindow), data->mainvbox);

        data->sbframe = gtk_frame_new(NULL);

        data->statusbar = gtk_statusbar_new();

        gtk_container_add(GTK_CONTAINER(data->sbframe), data->statusbar);

        data->mainhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        gtk_box_pack_start (GTK_BOX(data->mainvbox), data->mainhbox, TRUE, TRUE, 0);
        gtk_box_pack_end (GTK_BOX(data->mainvbox), data->sbframe, FALSE, FALSE, 0);

        data->leftvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        data->rightvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

        data->rvbframe = gtk_frame_new(NULL);

        gtk_container_add(GTK_CONTAINER(data->rvbframe), data->rightvbox);

        gtk_box_pack_end(GTK_BOX(data->mainhbox), data->rvbframe, FALSE, FALSE, 5);

        data->daframe = gtk_frame_new(NULL);
        gtk_widget_set_margin_start(data->daframe, 10);
        gtk_widget_set_margin_end(data->daframe, 10);
        gtk_widget_set_margin_top(data->daframe, 10);
        gtk_widget_set_margin_bottom(data->daframe, 5);

        data->drawarea = gtk_drawing_area_new();

        gtk_container_add(GTK_CONTAINER(data->daframe), data->drawarea);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->daframe, TRUE, TRUE, 5);

        data->i_label = gtk_label_new("\n\n");
        data->i_frame = gtk_frame_new(NULL);

        gtk_container_add(GTK_CONTAINER(data->i_frame), data->i_label);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->i_frame, FALSE, FALSE, 0);

        /* Esta estranheza toda de se usar uma scale ocorre apenas por motivos
           estéticos, no sentido em que gtk_radio_buttons não teriam o mesmo apelo... */

        data->plmd_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1, 2, 1);
        gtk_scale_set_draw_value(GTK_SCALE(data->plmd_scale), FALSE);
        gtk_scale_set_digits(GTK_SCALE(data->plmd_scale), 0);
        gtk_scale_add_mark(GTK_SCALE(data->plmd_scale), 1, GTK_POS_RIGHT, "<small>Cargas de prova</small>");
        gtk_scale_add_mark(GTK_SCALE(data->plmd_scale), 2, GTK_POS_RIGHT, "<small>Todas as cargas</small>");

        data->playbutton = gtk_toggle_button_new();
        gtk_container_add(GTK_CONTAINER(data->playbutton), gtk_image_new_from_file(PLAYPAUSE));
        gtk_widget_set_size_request(data->playbutton, 70, 70);

        gtk_widget_set_size_request(data->plmd_scale, -1, 70);

        data->plhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        data->tscale_spbutton = gtk_spin_button_new_with_range(DBL_EPSILON, 10.0, 1e-9);

        data->tse_label = gtk_label_new("Escala de tempo (s/frame):");

        data->tse_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        gtk_box_pack_start(GTK_BOX(data->tse_hbox), data->tse_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->tse_hbox), data->tscale_spbutton, FALSE, FALSE, 5);

        data->vectmodes_scale = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0, 2, 1);
        gtk_scale_set_draw_value(GTK_SCALE(data->vectmodes_scale), FALSE);
        gtk_scale_set_digits(GTK_SCALE(data->vectmodes_scale), 0);
        gtk_scale_add_mark(GTK_SCALE(data->vectmodes_scale), 0, GTK_POS_LEFT, "<small>Não Desenhar Vectores</small>");
        gtk_scale_add_mark(GTK_SCALE(data->vectmodes_scale), 1, GTK_POS_LEFT, "<small>Desenhar Direcções e Sentidos</small>");
        gtk_scale_add_mark(GTK_SCALE(data->vectmodes_scale), 2, GTK_POS_LEFT, "<small>Desenhar Vectores Completos</small>");
        gtk_widget_set_size_request(data->vectmodes_scale, -1, 70);

        gtk_box_pack_end(GTK_BOX(data->plhbox), data->plmd_scale, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(data->plhbox), data->vectmodes_scale, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(data->plhbox), data->tse_hbox, TRUE, TRUE, 5);


        gtk_box_set_center_widget(GTK_BOX(data->plhbox), data->playbutton);


        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->plhbox, FALSE, FALSE, 5);


        data->f_saver = gtk_file_chooser_dialog_new( "Guardar simulação...", GTK_WINDOW(data->mainwindow),
                                                     GTK_FILE_CHOOSER_ACTION_SAVE, "Guardar", GTK_RESPONSE_ACCEPT,
                                                     "Cancelar", GTK_RESPONSE_CLOSE, NULL                          );

        sf = gtk_file_filter_new();
        gtk_file_filter_add_pattern(sf, "*.sce");
        gtk_file_filter_set_name(sf, "Simulação (*.sce)");
        af = gtk_file_filter_new();
        gtk_file_filter_add_pattern(af, "*.*");
        gtk_file_filter_set_name(af, "Todos os ficheiros");

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(data->f_saver), sf);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(data->f_saver), af);

        data->f_loader = gtk_file_chooser_dialog_new( "Abrir simulação...", GTK_WINDOW(data->mainwindow),
                                                     GTK_FILE_CHOOSER_ACTION_OPEN, "Abrir", GTK_RESPONSE_ACCEPT,
                                                     "Cancelar", GTK_RESPONSE_CLOSE, NULL                        );

        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(data->f_loader), sf);
        gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(data->f_loader), af);

        data->uparrow = gtk_toggle_button_new_with_label("\U000025B2");
        data->leftarrow = gtk_toggle_button_new_with_label("\U000025C0");
        data->rightarrow = gtk_toggle_button_new_with_label("\U000025B6");
        data->downarrow = gtk_toggle_button_new_with_label("\U000025BC");

        data->offx_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->offx_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->offx_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->offx_entry), 20);
        data->offx_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->offx_label),"<small>Offset em x (m):</small>");
        data->offy_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->offy_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->offy_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->offy_entry), 20);
        data->offy_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->offy_label),"<small>Offset em y (m):</small>");

        gtk_widget_set_margin_start(data->rightarrow, 15);
        gtk_widget_set_margin_end(data->leftarrow, 15);


        data->uabox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_widget_set_size_request(data->uabox, 25, 25);

        gtk_box_set_center_widget(GTK_BOX(data->uabox), data->uparrow);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->uabox, FALSE, FALSE, 5);

        data->lrbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_widget_set_size_request(data->lrbox, -1, 25);

        data->off_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_box_pack_start(GTK_BOX(data->lrbox), data->leftarrow, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->off_box), data->offx_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(data->off_box), data->offx_entry, FALSE, FALSE, 0);


        gtk_box_pack_end(GTK_BOX(data->off_box), data->offy_entry, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(data->off_box), data->offy_label, FALSE, FALSE, 0);

        gtk_box_set_center_widget(GTK_BOX(data->lrbox), data->off_box);

        gtk_box_pack_end(GTK_BOX(data->lrbox), data->rightarrow, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->lrbox, FALSE, FALSE, 5);

        data->dabox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_widget_set_size_request(data->dabox, 25, 25);

        gtk_box_set_center_widget(GTK_BOX(data->dabox), data->downarrow);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->dabox, FALSE, FALSE, 5);

        data->vs_title = gtk_label_new("Escala de Visualização:");

        data->vis_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.99, 10.100, 0.001);
        gtk_scale_set_digits(GTK_SCALE(data->vis_scale), 3);
        gtk_scale_set_value_pos(GTK_SCALE(data->vis_scale), GTK_POS_RIGHT);

        data->vs_label = gtk_label_new("\n");

        gtk_widget_set_margin_top(data->vs_label, 23); //Visualmente, deve ser isto...
        gtk_widget_set_margin_top(data->vs_title, 23);

        data->vs_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

        gtk_box_pack_start(GTK_BOX(data->vs_box), data->vs_title, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(data->vs_box), data->vis_scale, TRUE, TRUE, 0);
        gtk_box_pack_start(GTK_BOX(data->vs_box), data->vs_label, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->vs_box, FALSE, FALSE, 0);

        data->minx_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->minx_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->minx_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->minx_entry), 20);
        data->minx_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->minx_label),"<small>x mínimo (m):</small>");

        data->miny_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->miny_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->miny_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->miny_entry), 20);
        data->miny_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->miny_label),"<small>y mínimo (m):</small>");

        data->maxx_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->maxx_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->maxx_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->maxx_entry), 20);
        data->maxx_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->maxx_label),"<small>x máximo (m):</small>");

        data->maxy_entry = gtk_entry_new();
        gtk_entry_set_has_frame(GTK_ENTRY(data->maxy_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->maxy_entry), 12);
        gtk_entry_set_max_length(GTK_ENTRY(data->maxy_entry), 20);
        data->maxy_label = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->maxy_label),"<small>y máximo (m):</small>");

        data->minmaxbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        gtk_box_pack_start(GTK_BOX(data->minmaxbox), data->minx_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(data->minmaxbox), data->minx_entry, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(data->minmaxbox), data->maxx_label, FALSE, FALSE, 0);
        gtk_box_pack_start(GTK_BOX(data->minmaxbox), data->maxx_entry, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(data->minmaxbox), data->maxy_entry, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(data->minmaxbox), data->maxy_label, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(data->minmaxbox), data->miny_entry, FALSE, FALSE, 0);
        gtk_box_pack_end(GTK_BOX(data->minmaxbox), data->miny_label, FALSE, FALSE, 0);

        gtk_box_pack_start(GTK_BOX(data->leftvbox), data->minmaxbox, FALSE, FALSE, 5);

        //bframe
	data->bframe = gtk_frame_new(NULL);
	data->bvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

	data->resetbutton = gtk_button_new_with_label ("Reiniciar Simulação");
        gtk_widget_set_size_request(data->resetbutton, 100, 25);
        gtk_box_pack_start(GTK_BOX(data->bvbox), data->resetbutton, TRUE, FALSE, 10);

	data->optionbutton = gtk_button_new_with_label ("Opções");
        gtk_widget_set_size_request(data->optionbutton, 100, 25);
	gtk_box_pack_start(GTK_BOX(data->bvbox), data->optionbutton, TRUE, FALSE, 10);

	data->savebutton = gtk_button_new_with_label ("Guardar");
        gtk_widget_set_size_request(data->savebutton, 100, 25);
	gtk_box_pack_start(GTK_BOX(data->bvbox), data->savebutton, TRUE, FALSE, 10);

	data->loadbutton = gtk_button_new_with_label ("Abrir");
        gtk_widget_set_size_request(data->loadbutton, 100, 25);
        gtk_box_pack_start(GTK_BOX(data->bvbox), data->loadbutton, TRUE, FALSE, 10);

	data->helpbutton = gtk_button_new_with_label ("Ajuda");

        gtk_widget_set_size_request(data->helpbutton, 100, 25);
	gtk_box_pack_start(GTK_BOX(data->bvbox), data->helpbutton, TRUE, FALSE, 10);

       	data->quitbutton = gtk_button_new_with_label("Sair");
        gtk_widget_set_size_request(data->quitbutton, 100, 25);
	gtk_box_pack_start(GTK_BOX(data->bvbox), data->quitbutton, TRUE, FALSE, 10);

	gtk_container_add(GTK_CONTAINER(data->bframe), data->bvbox);
	//bframe

        data->addq_entry = gtk_entry_new();

        data->addq_label = gtk_label_new("Adicionar cargas aleatórias:");

        gtk_entry_set_has_frame(GTK_ENTRY(data->addq_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->addq_entry), 4);
        gtk_entry_set_max_length(GTK_ENTRY(data->addq_entry), 8);

        data->addq_hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        gtk_box_pack_start(GTK_BOX(data->addq_hbox), data->addq_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->addq_hbox), data->addq_entry, FALSE, FALSE, 5);

        gtk_box_pack_end(GTK_BOX(data->rightvbox), data->addq_hbox, TRUE, FALSE, 0);


        data->chargeditorbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        data->chargeditorframe = gtk_frame_new(NULL);

  data->ce_label = gtk_label_new( "" );
  gtk_label_set_markup(GTK_LABEL(data->ce_label), "<small>De momento, não escolheu nenhuma carga.\n\n"
                                       "Escolha uma carga para modificar fazendo duplo clique"
                                       " ou seleccionando-a no menu acima</small>"                    );
        gtk_label_set_line_wrap(GTK_LABEL(data->ce_label), TRUE);

        data->ce_combobox = gtk_combo_box_text_new();

        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_combobox, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_label, FALSE, FALSE, 5);

        data->ce_namelabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_namelabel), "<small>Nome:</small>");
        data->ce_namentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_namentry), 32);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_namentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_namentry), 32);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_namelabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_namentry, FALSE, FALSE, 5);

        data->ce_colorlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_colorlabel), "<small>Cor:</small>");
        data->ce_colorbutton = gtk_color_button_new ();
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(data->ce_colorbutton), TRUE);
        gtk_color_button_set_title(GTK_COLOR_BUTTON(data->ce_colorbutton), "Escolha a cor que pretende para a carga...");
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_colorlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_colorbutton, FALSE, FALSE, 5);

        data->ce_qlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_qlabel), "<small>Carga:</small>");
        data->ce_qentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_qentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_qentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_qentry), 12);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_qlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_qentry, FALSE, FALSE, 5);

        data->ce_rlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_rlabel), "<small>Raio:</small>");
        data->ce_rentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_rentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_rentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_rentry), 12);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_rlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_rentry, FALSE, FALSE, 5);

        data->ce_mlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_mlabel), "<small>Massa:</small>");
        data->ce_mentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_mentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_mentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_mentry), 12);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_mlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_mentry, FALSE, FALSE, 5);

        data->ce_poshbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        data->ce_xvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        data->ce_xlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_xlabel), "<small>Posição em x:</small>");
        data->ce_xentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_xentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_xentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_xentry), 12);
        gtk_box_pack_start(GTK_BOX(data->ce_xvbox), data->ce_xlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_xvbox), data->ce_xentry, FALSE, FALSE, 5);

        data->ce_yvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        data->ce_ylabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_ylabel), "<small>Posição em y:</small>");
        data->ce_yentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_yentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_yentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_yentry), 12);
        gtk_box_pack_start(GTK_BOX(data->ce_yvbox), data->ce_ylabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_yvbox), data->ce_yentry, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(data->ce_poshbox), data->ce_xvbox, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_poshbox), data->ce_yvbox, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_poshbox, FALSE, FALSE, 5);

        data->ce_velhbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);

        data->ce_vxvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        data->ce_vxlabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_vxlabel), "<small>Velocidade em x:</small>");
        data->ce_vxentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_vxentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_vxentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_vxentry), 12);
        gtk_box_pack_start(GTK_BOX(data->ce_vxvbox), data->ce_vxlabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_vxvbox), data->ce_vxentry, FALSE, FALSE, 5);

        data->ce_vyvbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        data->ce_vylabel = gtk_label_new("");
        gtk_label_set_markup(GTK_LABEL(data->ce_vylabel), "<small>Velocidade em y:</small>");
        data->ce_vyentry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->ce_vyentry), 25);
        gtk_entry_set_has_frame(GTK_ENTRY(data->ce_vyentry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->ce_vyentry), 12);
        gtk_box_pack_start(GTK_BOX(data->ce_vyvbox), data->ce_vylabel, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_vyvbox), data->ce_vyentry, FALSE, FALSE, 5);

        gtk_box_pack_start(GTK_BOX(data->ce_velhbox), data->ce_vxvbox, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->ce_velhbox), data->ce_vyvbox, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_velhbox, FALSE, FALSE, 5);

        data->ce_delchargbutton = gtk_button_new_with_label("Apagar carga");

        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_delchargbutton, FALSE, FALSE, 5);

        data->ce_finallabel = gtk_label_new( "" );
        gtk_label_set_markup(GTK_LABEL(data->ce_finallabel), "<small>( Em todos os espaços acima, pode especificar a"
                                             "  unidade do input, sendo a sua ausência interpretada"
                                             " como a adopção de unidades SI. )</small>");
        gtk_label_set_line_wrap(GTK_LABEL(data->ce_finallabel), TRUE);

        gtk_box_pack_start(GTK_BOX(data->chargeditorbox), data->ce_finallabel, FALSE, FALSE, 5);

        gtk_container_add(GTK_CONTAINER(data->chargeditorframe), data->chargeditorbox);

        gtk_box_pack_start(GTK_BOX(data->rightvbox), data->chargeditorframe, TRUE, FALSE, 0);

        data->optiondialog = gtk_dialog_new_with_buttons( "Opções", GTK_WINDOW(data->mainwindow),
                                                          GTK_DIALOG_DESTROY_WITH_PARENT, "Repor predefinições",
                                                          GTK_RESPONSE_HELP, "Fechar", GTK_RESPONSE_CLOSE, NULL  );

        dcontent = gtk_dialog_get_content_area(GTK_DIALOG(data->optiondialog));

        data->odhbox1 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->cbtq_label = gtk_label_new("Cor das Cargas de Prova:");
        data->colorbtq = gtk_color_button_new ();
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(data->colorbtq), TRUE);
        gtk_color_button_set_title(GTK_COLOR_BUTTON(data->colorbtq), "Escolha a cor que pretende para as cargas de prova...");
        gtk_box_pack_start(GTK_BOX(data->odhbox1), data->cbtq_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox1), data->colorbtq, FALSE, FALSE, 5);

        data->odhbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->cbvect_label = gtk_label_new("Cor dos Vectores:");
        data->colorbvect = gtk_color_button_new ();
        gtk_color_chooser_set_use_alpha(GTK_COLOR_CHOOSER(data->colorbvect), TRUE);
        gtk_color_button_set_title(GTK_COLOR_BUTTON(data->colorbvect), "Escolha a cor que pretende para os vectores...");
        gtk_box_pack_start(GTK_BOX(data->odhbox2), data->cbvect_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox2), data->colorbvect, FALSE, FALSE, 5);

        data->odhbox3 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->tqr_label = gtk_label_new("Raio das Cargas de Prova (píxeis no ecrã):");
        data->tqr_entry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->tqr_entry), 20);
        gtk_entry_set_has_frame(GTK_ENTRY(data->tqr_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->tqr_entry), 11);
        gtk_box_pack_start(GTK_BOX(data->odhbox3), data->tqr_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox3), data->tqr_entry, FALSE, FALSE, 5);

        data->odhbox4 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->tqq_label = gtk_label_new("Carga das Cargas de Prova (C):");
        data->tqq_entry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->tqq_entry), 20);
        gtk_entry_set_has_frame(GTK_ENTRY(data->tqq_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->tqq_entry), 11);
        gtk_box_pack_start(GTK_BOX(data->odhbox4), data->tqq_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox4), data->tqq_entry, FALSE, FALSE, 5);

        data->odhbox5 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->tqm_label = gtk_label_new("Massa das Cargas de Prova (kg):");
        data->tqm_entry = gtk_entry_new ();
        gtk_entry_set_max_length(GTK_ENTRY(data->tqm_entry), 20);
        gtk_entry_set_has_frame(GTK_ENTRY(data->tqm_entry), TRUE);
        gtk_entry_set_width_chars(GTK_ENTRY(data->tqm_entry), 11);
        gtk_box_pack_start(GTK_BOX(data->odhbox5), data->tqm_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox5), data->tqm_entry, FALSE, FALSE, 5);

        data->odhbox6 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->numvects_label = gtk_label_new("Número de Vectores Por Lado:");
        data->numvects_sb = gtk_spin_button_new_with_range(1, 100, 1);
        gtk_box_pack_start(GTK_BOX(data->odhbox6), data->numvects_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox6), data->numvects_sb, FALSE, FALSE, 5);

        data->odhbox7 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->vadj_label = gtk_label_new("Ajuste do Comprimento dos Vectores:");
        data->vadj_scale = gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, 0.5, 250.0, 1.0);
        gtk_scale_add_mark(GTK_SCALE(data->vadj_scale), 10.0, GTK_POS_BOTTOM, "10");
        gtk_scale_add_mark(GTK_SCALE(data->vadj_scale), 50.0, GTK_POS_TOP, "50");
        gtk_scale_add_mark(GTK_SCALE(data->vadj_scale), 75.0, GTK_POS_BOTTOM, "75");
        gtk_scale_add_mark(GTK_SCALE(data->vadj_scale), 100.0, GTK_POS_TOP, "100");
        gtk_widget_set_size_request(data->vadj_scale, 200, -1);
        gtk_scale_set_draw_value(GTK_SCALE(data->vadj_scale), TRUE);
        gtk_scale_set_value_pos(GTK_SCALE(data->vadj_scale), GTK_POS_RIGHT);
        gtk_box_pack_start(GTK_BOX(data->odhbox7), data->vadj_label, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(data->odhbox7), data->vadj_scale, FALSE, FALSE, 5);

        data->odhbox8 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
        data->tqc_b = gtk_check_button_new_with_label("Verificar Colisões das Cargas de Prova");
        data->sl_b = gtk_check_button_new_with_label("Guardar Último Estado do Programa");
        gtk_box_pack_start(GTK_BOX(data->odhbox8), data->tqc_b, FALSE, FALSE, 10);
        gtk_box_pack_start(GTK_BOX(data->odhbox8), data->sl_b, FALSE, FALSE, 10);

        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox1, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox2, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox3, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox4, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox5, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox6, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox7, FALSE, FALSE, 5);
        gtk_box_pack_start(GTK_BOX(dcontent), data->odhbox8, FALSE, FALSE, 5);


        gtk_window_set_icon_from_file(GTK_WINDOW(data->optiondialog), ICON, &p);

        data->mbox1 = gtk_message_dialog_new( GTK_WINDOW(data->mainwindow), GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                             GTK_MESSAGE_WARNING, GTK_BUTTONS_NONE, "A configuração que está a ser aberta foi "
                                             "gravada em modo debug.\n\nDeseja activar o modo debug?\n\n (Caso não tenha bem a"
                                             " certeza do que se trata, pode igonrar este aviso com toda a segurança.)\n\n"     );

        gtk_dialog_add_buttons(GTK_DIALOG(data->mbox1), "Sim", GTK_RESPONSE_YES, "Não", GTK_RESPONSE_NONE, NULL);

        data->mbox2 = gtk_message_dialog_new( GTK_WINDOW(data->mainwindow), GTK_DIALOG_DESTROY_WITH_PARENT,
                                              GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE, "Para aumentar a zona de visualização,"
                                              "arraste os lados.\nPara criar cargas de prova, faça duplo clique com o botão"
                                              "esquerdo do rato.\nPara criar ou apagar cargas normais, faça o mesmo, mas com"
                                              "o botão direito.\nPara seleccionar uma carga, faça duplo clique com o botão"
                                              "esquerdo do rato sobre ela, ou seleccione-a na combo_box, no topo, à direita\n"
                                              "Quando introduzir valores através do teclado, prima enter no fim.\n"
                                              "Para mais informações, consulte o documento pdf que acompanha o programa."      );
        gtk_dialog_add_buttons(GTK_DIALOG(data->mbox2), "OK", GTK_RESPONSE_CLOSE, NULL);

        gtk_window_set_title(GTK_WINDOW(data->mbox2), "Ajuda");
        gtk_window_set_icon_from_file(GTK_WINDOW(data->mbox2), ICON, &p);

        data->lvbframe = gtk_frame_new(NULL);

        gtk_container_add(GTK_CONTAINER(data->lvbframe), data->leftvbox);

        gtk_box_pack_start(GTK_BOX(data->mainhbox), data->lvbframe, TRUE, TRUE, 5);
	gtk_box_pack_end(GTK_BOX(data->mainhbox), data->bframe, FALSE, FALSE, 0);

        gtk_widget_set_events( data->drawarea, GDK_BUTTON_PRESS_MASK | GDK_SCROLL_MASK |
                               GDK_BUTTON_RELEASE_MASK | GDK_POINTER_MOTION_MASK         );

        g_signal_connect (G_OBJECT(data->mainwindow), "destroy", G_CALLBACK (quit), data);
        g_signal_connect (G_OBJECT(data->drawarea), "draw", G_CALLBACK (draw_stuff), data);
        g_signal_connect (G_OBJECT(data->drawarea), "motion_notify_event", G_CALLBACK (mouse_pan), data);
        g_signal_connect (G_OBJECT(data->drawarea), "button_press_event", G_CALLBACK (mouse_pan), data);
        g_signal_connect (G_OBJECT(data->drawarea), "button_release_event", G_CALLBACK (mouse_pan), data);
        g_signal_connect (G_OBJECT(data->drawarea), "scroll_event", G_CALLBACK (scroll), data);
        g_signal_connect (G_OBJECT(data->tscale_spbutton), "value_changed", G_CALLBACK(change_timescale), data);
        g_signal_connect (G_OBJECT(data->vis_scale), "value_changed", G_CALLBACK (v_scale_change), data);
        g_signal_connect (G_OBJECT(data->plmd_scale), "value_changed", G_CALLBACK(play_mode_change), data);
        g_signal_connect (G_OBJECT(data->vectmodes_scale), "value_changed", G_CALLBACK(vector_mode_change), data);
        g_signal_connect (G_OBJECT(data->quitbutton), "clicked", G_CALLBACK (quit), data);
        g_signal_connect (G_OBJECT(data->resetbutton), "clicked", G_CALLBACK (reset_button), data);
        g_signal_connect (G_OBJECT(data->savebutton), "clicked", G_CALLBACK (save_button), data);
        g_signal_connect (G_OBJECT(data->loadbutton), "clicked", G_CALLBACK (load_button), data);
        g_signal_connect (G_OBJECT(data->helpbutton), "clicked", G_CALLBACK (show_help), data);
        g_signal_connect (G_OBJECT(data->optionbutton), "clicked", G_CALLBACK (show_options), data);
        g_signal_connect (G_OBJECT(data->offx_entry), "activate", G_CALLBACK (offset_entries), data);
        g_signal_connect (G_OBJECT(data->offy_entry), "activate", G_CALLBACK (offset_entries), data);
        g_signal_connect (G_OBJECT(data->colorbtq), "color_set", G_CALLBACK (select_test_charge_color), data);
        g_signal_connect (G_OBJECT(data->colorbvect), "color_set", G_CALLBACK (select_vector_color), data);
        g_signal_connect (G_OBJECT(data->tqr_entry), "activate", G_CALLBACK (tq_properties_entries), data);
        g_signal_connect (G_OBJECT(data->tqm_entry), "activate", G_CALLBACK (tq_properties_entries), data);
        g_signal_connect (G_OBJECT(data->tqq_entry), "activate", G_CALLBACK (tq_properties_entries), data);
        g_signal_connect (G_OBJECT(data->minx_entry), "activate", G_CALLBACK (minmax_entries), data);
        g_signal_connect (G_OBJECT(data->miny_entry), "activate", G_CALLBACK (minmax_entries), data);
        g_signal_connect (G_OBJECT(data->maxx_entry), "activate", G_CALLBACK (minmax_entries), data);
        g_signal_connect (G_OBJECT(data->maxy_entry), "activate", G_CALLBACK (minmax_entries), data);
        g_signal_connect (G_OBJECT(data->mbox2), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (G_OBJECT(data->optiondialog), "delete-event", G_CALLBACK (gtk_widget_hide_on_delete), NULL);
        g_signal_connect (G_OBJECT(data->mbox2), "response", G_CALLBACK (hide), NULL);
        g_signal_connect (G_OBJECT(data->optiondialog), "response", G_CALLBACK (option_response), data);
        g_signal_connect (G_OBJECT(data->numvects_sb), "value_changed", G_CALLBACK(change_numvects), data);
        g_signal_connect (G_OBJECT(data->vadj_scale), "value_changed", G_CALLBACK(vectadjust_change), data);
        g_signal_connect (G_OBJECT(data->tqc_b), "toggled", G_CALLBACK(tqc_toggle), data);
        g_signal_connect (G_OBJECT(data->sl_b), "toggled", G_CALLBACK(sl_toggle), data);
        g_signal_connect (G_OBJECT(data->addq_entry), "activate", G_CALLBACK (add_random_charges), data);
        g_signal_connect (G_OBJECT(data->ce_combobox), "changed", G_CALLBACK (change_qedit), data);
        g_signal_connect (G_OBJECT(data->ce_colorbutton), "color_set", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_mentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_namentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_rentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_qentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_vxentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_vyentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_xentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_yentry), "activate", G_CALLBACK (chargeditorinput), data);
        g_signal_connect (G_OBJECT(data->ce_delchargbutton), "clicked", G_CALLBACK (chargeditorinput), data);

        data->to_motion = g_timeout_add(10, (GSourceFunc) timeout, data);

        data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);

        gtk_widget_show_all (data->mainwindow);
}

void init_data (pdata *data)
/* Lê o último estado do programa, para se retomar onde se ficou,
   e as configurações, para conforto do utilizador, inicializando
   ambos os conjuntos de informação se se der o caso de a leitura
   não ser bem-sucedida.                                          */
{
    if (!load_sim(data, LASTSTATE))
    {
        if(check_debug(data->state))
        {
                gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS,
                                    "Erro na leitura do ficheiro '" LASTSTATE "'! Adoptando valores pré-definidos...");
                g_source_remove(data->to_clean);
                data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
        reset_data(data, RES_SIM);
    }
    if (!load_config(data, CONFIG))
    {
        if(check_debug(data->state))
        {
                gtk_statusbar_push( GTK_STATUSBAR(data->statusbar), WARNINGS,
                                    "Erro na leitura do ficheiro '" CONFIG "'! Adoptando configurações pré-definidas..." );
                g_source_remove(data->to_clean);
                data->to_clean = g_timeout_add_seconds(3, (GSourceFunc) cleanstatusbar, data->statusbar);
        }
        reset_data(data, RES_CONFIG);
    }
}


/* ---------------------------------------------------------------- */
/* ---------------------------- MAIN ------------------------------ */
/* ---------------------------------------------------------------- */

int main (int argc, char **argv)
{
    int i;
        pdata data = INITIALVALUES;
        srand(time(NULL));
        gtk_init(NULL, NULL);
        for (i = 1; i < argc; ++i)
        {
            if (!strcmp(argv[i], "-debug"))
            {
                data.state |= SURE;
                break;
            }
        }
        if (i == 1 && argc > 2)
        {
            create_style_from_file(argv[2]);
        }
        else if (argc > 1)
        {
            create_style_from_file(argv[1]);
        }
        printf("\n\n--- Simulador de Campos Eléctricos ---\n\n");
        printf("Versão: %s\n\nPor: Gonçalo Vaz e Nuno Fernandes\n\n", VERSION);
        if (check_debug(data.state))
        {
                printf("MODO DE DEBUG\n\n");
        }
        init_widgets (&data);
        gtk_widget_show_all (data.mainwindow);
        init_data(&data);
        gtk_main ();
        free(data.qs);
        free(data.tqs);
        return 0;
}
