#include <iostream>
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>
using namespace std;

const char* NOM_FICHIER = "texte_10M.txt";        // changez en "texte.txt" ou "texte_10M.txt"
const char* NOM_SORTIE  = "texte_modifie.txt";     // fichier créé après le remplacement
const int REPETITIONS = 1;                         // nombre de répétitions de chaque méthode

// Un mot du fichier = une zone [debut, fin) dans le vector<char> du texte
struct Mot {
    size_t debut;
    size_t fin;
};

vector<char> lireMot() {
    vector<char> mot;
    char c;
    cin >> ws;
    while (cin.get(c) && c != '\n' && c != ' ' && c != '\r')
        mot.push_back(c);
    return mot;
}

// Lecture du fichier en un seul bloc (rapide)
vector<char> lireFichier(const char* nom) {
    ifstream f(nom, ios::binary | ios::ate);
    size_t taille = f.tellg();
    f.seekg(0);
    vector<char> v(taille);
    f.read(v.data(), taille);
    return v;
}

// Caractères qui séparent deux mots dans le fichier
bool estSeparateur(char c) {
    return c == ' ' || c == '\n' || c == '\r' || c == '\t' ||
           c == '.' || c == ',' || c == ';' || c == ':' ||
           c == '!' || c == '?';
}

// Découpe le texte en mots : on mémorise le début et la fin de chaque mot
vector<Mot> decouperMots(const vector<char>& texte) {
    vector<Mot> mots;
    mots.reserve(texte.size() / 5);
    size_t i = 0;
    while (i < texte.size()) {
        while (i < texte.size() && estSeparateur(texte[i]))
            i++;
        size_t debut = i;
        while (i < texte.size() && !estSeparateur(texte[i]))
            i++;
        if (i > debut)
            mots.push_back({debut, i});
    }
    return mots;
}

// Affiche un vector<char>
void afficher(const vector<char>& v) {
    for (size_t i = 0; i < v.size(); i++)
        cout << v[i];
}

// ---------- Méthode 1 : avec search (équivalent de find pour un vector) ----------
bool contientFind(const vector<char>& texte, const Mot& m, const vector<char>& cherche) {
    auto debut = texte.begin() + m.debut;
    auto fin = texte.begin() + m.fin;
    return search(debut, fin, cherche.begin(), cherche.end()) != fin;
}

// ---------- Méthode 2 : from scratch ----------
bool contientScratch(const vector<char>& texte, const Mot& m, const vector<char>& cherche) {
    for (size_t i = m.debut; i + cherche.size() <= m.fin; i++) {
        size_t k = 0;
        while (k < cherche.size() && texte[i + k] == cherche[k])
            k++;
        if (k == cherche.size())
            return true;
    }
    return false;
}

// ---------- Remplacement : dans chaque mot, 'cherche' est remplacé par 'remplace' ----------
// Renvoie le nouveau texte. Les séparateurs (espaces, ponctuation, retours à la ligne)
// sont recopiés tels quels.
vector<char> remplacerDansMots(const vector<char>& texte, const vector<Mot>& mots,
                               const vector<char>& cherche, const vector<char>& remplace,
                               size_t& nbMotsModifies, size_t& nbRemplacements,
                               vector<vector<char> >& avant, vector<vector<char> >& apres) {
    vector<char> resultat;
    resultat.reserve(texte.size() + texte.size() / 10);
    nbMotsModifies = 0;
    nbRemplacements = 0;
    size_t derniere = 0;                           // fin du mot précédent

    for (size_t k = 0; k < mots.size(); k++) {
        const Mot& m = mots[k];

        // 1) on recopie ce qui sépare le mot précédent de celui-ci
        for (size_t i = derniere; i < m.debut; i++)
            resultat.push_back(texte[i]);

        // 2) on recopie le mot en remplaçant 'cherche' par 'remplace'
        size_t tailleAvant = resultat.size();
        size_t remplacesDansCeMot = 0;
        size_t i = m.debut;
        while (i < m.fin) {
            // est-ce que 'cherche' commence ici ?
            bool trouve = (i + cherche.size() <= m.fin);
            for (size_t j = 0; trouve && j < cherche.size(); j++)
                if (texte[i + j] != cherche[j])
                    trouve = false;

            if (trouve) {
                for (size_t j = 0; j < remplace.size(); j++)
                    resultat.push_back(remplace[j]);
                i += cherche.size();               // on saute le morceau remplacé
                remplacesDansCeMot++;
            } else {
                resultat.push_back(texte[i]);
                i++;
            }
        }

        if (remplacesDansCeMot > 0) {
            nbMotsModifies++;
            nbRemplacements += remplacesDansCeMot;
            if (avant.size() < 5) {                // on garde 5 exemples à afficher
                avant.push_back(vector<char>(texte.begin() + m.debut, texte.begin() + m.fin));
                apres.push_back(vector<char>(resultat.begin() + tailleAvant, resultat.end()));
            }
        }
        derniere = m.fin;
    }

    // 3) ce qui reste après le dernier mot (retour à la ligne final, ...)
    for (size_t i = derniere; i < texte.size(); i++)
        resultat.push_back(texte[i]);

    return resultat;
}

int main() {
    ifstream test(NOM_FICHIER);
    if (!test) {
        cout << "Erreur : impossible d'ouvrir " << NOM_FICHIER << endl;
        return 1;
    }
    test.close();

    vector<char> texte = lireFichier(NOM_FICHIER);
    vector<Mot> mots = decouperMots(texte);
    cout << "Taille du fichier : " << texte.size() << " caracteres" << endl;
    cout << "Nombre de mots dans le fichier : " << mots.size() << endl;

    cout << "Mot a chercher : ";
    vector<char> cherche = lireMot();
    if (cherche.empty()) {
        cout << "Mot vide." << endl;
        return 1;
    }

    size_t nbFind = 0, nbScratch = 0;

    // ----- Méthode find (search) : REPETITIONS passes sur tous les mots -----
    auto d1 = chrono::high_resolution_clock::now();
    for (int r = 0; r < REPETITIONS; r++) {
        nbFind = 0;
        for (size_t k = 0; k < mots.size(); k++)
            if (contientFind(texte, mots[k], cherche))
                nbFind++;
    }
    auto f1 = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> t1 = f1 - d1;

    // ----- Méthode scratch : REPETITIONS passes sur tous les mots -----
    auto d2 = chrono::high_resolution_clock::now();
    for (int r = 0; r < REPETITIONS; r++) {
        nbScratch = 0;
        for (size_t k = 0; k < mots.size(); k++)
            if (contientScratch(texte, mots[k], cherche))
                nbScratch++;
    }
    auto f2 = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> t2 = f2 - d2;

    cout << "Methode find    : " << nbFind << " mots trouves"
         << " | total " << t1.count() << " ms"
         << " | moyenne " << t1.count() / REPETITIONS << " ms" << endl;
    cout << "Methode scratch : " << nbScratch << " mots trouves"
         << " | total " << t2.count() << " ms"
         << " | moyenne " << t2.count() / REPETITIONS << " ms" << endl;

    // ================= Remplacement =================
    if (nbScratch == 0) {
        cout << "Aucun mot a modifier." << endl;
        return 0;
    }

    char reponse;
    cout << "\nVoulez-vous remplacer \"";
    afficher(cherche);
    cout << "\" dans ces " << nbScratch << " mots ? (o/n) : ";
    cin >> reponse;
    if (reponse != 'o' && reponse != 'O') {
        cout << "Aucune modification." << endl;
        return 0;
    }

    cout << "Remplacer \"";
    afficher(cherche);
    cout << "\" par : ";
    vector<char> remplace = lireMot();
    if (remplace.empty()) {
        cout << "Sous-chaine vide, aucune modification." << endl;
        return 1;
    }

    size_t nbMotsModifies = 0, nbRemplacements = 0;
    vector<vector<char> > avant, apres;

    auto d3 = chrono::high_resolution_clock::now();
    vector<char> nouveau = remplacerDansMots(texte, mots, cherche, remplace,
                                             nbMotsModifies, nbRemplacements,
                                             avant, apres);
    auto f3 = chrono::high_resolution_clock::now();
    chrono::duration<double, milli> t3 = f3 - d3;

    // Écriture du résultat dans un nouveau fichier (le fichier d'origine reste intact)
    ofstream sortie(NOM_SORTIE, ios::binary);
    sortie.write(nouveau.data(), nouveau.size());
    sortie.close();

    cout << "\nMots modifies : " << nbMotsModifies << endl;
    cout << "Remplacements effectues : " << nbRemplacements << endl;
    cout << "Temps du remplacement : " << t3.count() << " ms" << endl;
    cout << "Exemples :" << endl;
    for (size_t k = 0; k < avant.size(); k++) {
        cout << "  ";
        afficher(avant[k]);
        cout << " -> ";
        afficher(apres[k]);
        cout << endl;
    }
    cout << "Resultat enregistre dans " << NOM_SORTIE << endl;

    return 0;
}
