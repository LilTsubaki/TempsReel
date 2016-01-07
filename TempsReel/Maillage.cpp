#include "stdafx.h"
#include "maillage.h"
#include <iostream>
#include <algorithm>
#include <cmath>

const std::vector<glm::vec3>& Maillage::getGeom() const
{
    return geom;
}

const std::vector<int>& Maillage::getTopo() const
{
    return topo;
}

const std::vector<glm::vec3>& Maillage::getNormales() const
{
    return normales;
}

const std::vector<int>& Maillage::getNormalIds() const
{
    return normalIds;
}
Maillage::Maillage()
{
}

Maillage::Maillage(const Maillage &m):geom(m.geom),topo(m.topo),normales(m.normales),normalIds(m.normalIds)
{
    std::cout<<"copie "<<geom.size()<<std::endl;
}

void Maillage::Merge(Maillage figure2)
{
	std::vector<int> changement;
    int temp;
    //on parcours la liste des points de la seconde figure
    //on vérifie si il y a des points communs entre les deux figures
    for(int p1 = 0; p1 < figure2.geom.size(); p1++)
    {
//        temp = this->ComparePoint(figure2.geom.at(p1));
//        si le point n'existe pas deja alors on l'ajoute à la geométrie de la premiere figure
//        on stock également le nouvel indice de ce point
//        if(temp == -1)
//        {
            this->geom.push_back(figure2.geom.at(p1));
            temp = this->geom.size()-1;
        //}
        //std::cout << "l'indice " << p1 << "est maintenant l'indice " << temp << std::endl;
        changement.push_back(temp);
    }

    //on ajoute a la premiere topo la seconde en mettant a jour les indices
    for(int i = 0; i < figure2.topo.size(); i++)
    {
        this->topo.push_back(changement.at(figure2.topo.at(i)));
    }
}




void Maillage::translate(const glm::vec3 &t, glm::vec3 &min, glm::vec3 &max)
{
    min=glm::vec3(std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity(),std::numeric_limits<float>::infinity());
    max=glm::vec3(std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest(),std::numeric_limits<float>::lowest());
    for(int i=0;i<geom.size();++i){
        geom[i]+=t;
        //std::cout<<geom[i].x()<<std::endl;
        min.x=std::min(min.x,geom[i].x);
        min.y=std::min(min.y,geom[i].y);
        min.z=std::min(min.z,geom[i].z);

        max.x=std::max(max.x,geom[i].x);
        max.y=std::max(max.y,geom[i].y);
        max.z=std::max(max.z,geom[i].z);

    }
    /*min = glm::vec3(min.x+t.x(), min.y+t.y(),min.z+t.z());
    max = glm::vec3(max.x+t.x(), max.y+t.y(),max.z+t.z());*/
}

Maillage Maillage::Rotation(const double matrice[3][3])
{
	std::vector<glm::vec3> geom2;
	glm::vec3 temp;
    for (int i = 0; i < geom.size(); ++i)
    {
         temp = geom.at(i);
         temp.x  = (temp.x * matrice[0][0] + temp.y * matrice[0][1] + temp.z * matrice[0][2]);
         temp.y = (temp.x * matrice[2][0] + temp.y * matrice[2][1] + temp.z * matrice[2][2]);
         temp.z = (temp.x * matrice[1][0] + temp.y * matrice[1][1] + temp.z * matrice[1][2]);
         geom2.push_back(temp);
    }

    Maillage * sphere = new Maillage(geom2, topo, normales);
    return *sphere;
}




Maillage::~Maillage()
{

}

void Maillage::geometry(const glm::vec3 &center, const char* obj, glm::vec3 &min, glm::vec3 &max) {
	glm::vec3 minVal(1E100, 1E100, 1E100), maxVal(-1E100, -1E100, -1E100);
	FILE* f;
	fopen_s(&f, obj, "rt");	
	//= fopen(obj, "r");

    while (!feof(f)) {
        char line[255];
        fgets(line, 255, f);
        if (line[0]=='v' && line[1]==' ') {
			glm::vec3 vec;
            sscanf_s(line, "v %f %f %f\n", &vec[0], &vec[2], &vec[1]);
            vec[2] = -vec[2];
			glm::vec3 p = vec + center;
			//std::cout << p.x << " " << p.y << " " << p.z << std::endl;
            geom.push_back(p);
            maxVal[0] = std::max(maxVal[0], p[0]);
            maxVal[1] = std::max(maxVal[1], p[1]);
            maxVal[2] = std::max(maxVal[2], p[2]);
            minVal[0] = std::min(minVal[0], p[0]);
            minVal[1] = std::min(minVal[1], p[1]);
            minVal[2] = std::min(minVal[2], p[2]);
            min = glm::vec3(minVal[0], minVal[1], minVal[2]);
            max = glm::vec3(maxVal[0], maxVal[1], maxVal[2]);
        }
        if (line[0]=='v' && line[1]=='n') {
			glm::vec3 vec;
            sscanf_s(line, "vn %f %f %f\n", &vec[0], &vec[2], &vec[1]);
            vec[2] = -vec[2];
            normales.push_back(vec);
        }
        if (line[0]=='f') {
            int i0, i1, i2;
            int j0,j1,j2;
            int k0,k1,k2;
            sscanf_s(line, "f %u//%u %u//%u %u//%u\n", &i0, &k0, &i1, &k1, &i2, &k2 );
            topo.push_back(i0-1);
            topo.push_back(i1-1);
            topo.push_back(i2-1);
            normalIds.push_back(k0-1);
            normalIds.push_back(k1-1);
            normalIds.push_back(k2-1);
        }

    }

    /*boundingSphere.C = 0.5*(minVal+maxVal);
    boundingSphere.R = sqrt((maxVal-minVal).sqrNorm())*0.5;*/

    fclose(f);
}

std::vector<float> Maillage::getAllPoints()
{
	std::vector<float> retour;
	for (int i = 0; i < topo.size(); i++)
	{
		retour.push_back(geom[topo[i]].x);
		retour.push_back(geom[topo[i]].y);
		retour.push_back(geom[topo[i]].z);
	}
	return retour;
}

std::vector<float> Maillage::getallNormals()
{
	std::vector<float> retour;
	for (int i = 0; i < normalIds.size(); i++)
	{
		retour.push_back(normales[normalIds[i]].x);
		retour.push_back(normales[normalIds[i]].y);
		retour.push_back(normales[normalIds[i]].z);
	}
	return retour;
}
