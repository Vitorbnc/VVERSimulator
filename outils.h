#ifndef OUTILS_H
#define OUTILS_H
/* Utilidades gerais, principalmente para criar a UI
 * 
 */


extern const int DEFAULT_DISPLAY_W;
extern const int DEFAULT_DISPLAY_H;

extern ImFont *fonte, *fonteBold;

extern int display_w, display_h; 

bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height);

#define constrain(x,min,max)  (x>max)?(max):((x<min)?(min):(x))
#define constrainInPlace(x,min,max)  (x>max)?(x=max):((x<min)?(x=min):(x))
#define mapVal(in, inLow,inHigh,outLow,outHigh) (outLow+(outHigh-outLow)*in/(inHigh-inLow))


struct Image {
	const char* path;
	//width
	int w;
	//height
	int h;
	//texture id
	GLuint id=0;

	float aspectRatio;

	bool load(){
		bool res = LoadTextureFromFile(path, &id, &w, &h);
		//id = (void*)(intptr_t) texture;
		aspectRatio = w/h;
		return res;
	}

};

// Códigos copiados, não alterados ficam nesta seção ##########################

// Simple helper function to load an image into a OpenGL texture with common settings
bool LoadTextureFromFile(const char* filename, GLuint* out_texture, int* out_width, int* out_height)
{
    // Load from file
    int image_width = 0;
    int image_height = 0;
    unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
    if (image_data == NULL)
        return false;

    // Create a OpenGL texture identifier
    GLuint image_texture;
    glGenTextures(1, &image_texture);
    glBindTexture(GL_TEXTURE_2D, image_texture);

    // Setup filtering parameters for display
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Upload pixels into texture
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, image_width, image_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image_data);
    stbi_image_free(image_data);

    *out_texture = image_texture;
    *out_width = image_width;
    *out_height = image_height;

    return true;
}
#if SHOW_MOUSE_POS
static void ShowExampleAppSimpleOverlay(bool* p_open)
{
    const float DISTANCE = 10.0f;
    static int corner = 1;
    ImGuiIO& io = ImGui::GetIO();
    if (corner != -1)
    {
        ImVec2 window_pos = ImVec2((corner & 1) ? io.DisplaySize.x - DISTANCE : DISTANCE, (corner & 2) ? io.DisplaySize.y - DISTANCE : DISTANCE);
        ImVec2 window_pos_pivot = ImVec2((corner & 1) ? 1.0f : 0.0f, (corner & 2) ? 1.0f : 0.0f);
        ImGui::SetNextWindowPos(window_pos, ImGuiCond_Always, window_pos_pivot);
    }
    ImGui::SetNextWindowBgAlpha(0.35f); // Transparent background
    ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav;
    if (corner != -1)
        window_flags |= ImGuiWindowFlags_NoMove;
    if (ImGui::Begin("Example: Simple overlay", p_open, window_flags))
    {
        ImGui::Text("Simple overlay\n" "in the corner of the screen.\n" "(right-click to change position)");
        ImGui::Separator();
        if (ImGui::IsMousePosValid())
            ImGui::Text("Mouse Position: (%.1f,%.1f)", io.MousePos.x, io.MousePos.y);
        else
            ImGui::Text("Mouse Position: <invalid>");
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Custom",       NULL, corner == -1)) corner = -1;
            if (ImGui::MenuItem("Top-left",     NULL, corner == 0)) corner = 0;
            if (ImGui::MenuItem("Top-right",    NULL, corner == 1)) corner = 1;
            if (ImGui::MenuItem("Bottom-left",  NULL, corner == 2)) corner = 2;
            if (ImGui::MenuItem("Bottom-right", NULL, corner == 3)) corner = 3;
            if (p_open && ImGui::MenuItem("Close")) *p_open = false;
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}
#endif
//###################  fim da seção ###############


//Rotaciona um vetor 2d
inline ImVec2 ImRotate(const ImVec2 &v, float cos_a,float sin_a){
	return ImVec2(v.x*cos_a -v.y*sin_a,   v.x*sin_a+ v.y*cos_a);
}

//nline ImVec2 operator+(const ImVec &a, const ImVec &b) {return ImVec2(a.x()+b.x(),a.y(),b.y());}

inline ImVec2 addImVec2(const ImVec2 &a, const ImVec2 &b) {return ImVec2( a.x+b.x, a.y+b.y );}

inline void addImage(ImTextureID texId,ImVec2 center, ImVec2 size){
	ImGui::GetWindowDrawList()->AddImageQuad(texId,
		ImVec2(center.x-.5*size.x,center.y-.5*size.y),
		ImVec2(center.x+.5*size.x,center.y-.5*size.y),
		ImVec2(center.x+.5*size.x,center.y+.5*size.y),
		ImVec2(center.x-.5*size.x,center.y+.5*size.y)
		);
}

//Add rotated image to draw list
void addImageRotated(ImTextureID texId, ImVec2 center, ImVec2 size, float angle){
	ImDrawList *drawList = ImGui::GetWindowDrawList();

	float cos_a = cosf(angle);
	float sin_a = sinf(angle);

	//Precisamos de metade do tamanho para os vetores que apontam do centro aos vértices
	//O plano tem (0,0) no canto sup esquerdo, Y cresce para baixo, X cresce para direita
	//Vértices começam do sup. esq. e vao no sentido horário
	ImVec2 v[4] = {
		addImVec2(center, ImRotate(ImVec2(-.5*size.x, -.5*size.y),cos_a,sin_a)), // Canto sup. esq.
		addImVec2(center,ImRotate(ImVec2(.5*size.x , -.5*size.y), cos_a, sin_a)), //Canto sup dir
		addImVec2(center,ImRotate(ImVec2(.5*size.x , .5*size.y), cos_a, sin_a)), //Canto inf dir
		addImVec2(center,ImRotate(ImVec2(-.5*size.x , .5*size.y), cos_a, sin_a)) //Canto inf esq
	};
	drawList->AddImageQuad(texId,v[0],v[1],v[2],v[3]);
}

//Funções inline são copiadas e coladas no lugar onde são chamadas. Boas para códigos simples
//Funções normais implicam em uma sobrecarga para alocar memória e copiar os valores dos parâmetros
//Margem esquerda, proporcional à largura da janela
inline void marginLeft(int size){ 
	ImGui::SetCursorPosX(((float)size)*0.1 * display_w);
	//ImGui::SetCursorPosX(((float)size)*0.1 * ImGui::GetIO().DisplaySize.x); // Alternativa, também funciona
}

//Espaço, geralmente é vertical, mas varia com o contexto
inline void spacing(int size=1){
	for(int i=0;i<size;i++) ImGui::Spacing();
}
//Espaço horizontal
inline void hSpacing(int size){
	for(int i=0;i<size;i++) {
		ImGui::SameLine();
		ImGui::Spacing();
	}
	ImGui::SameLine();
}

void outputDoubleWindow(const char* label, const char* units,const double value, int referenceId, int x, int y, int width=100,int height=90,ImVec4 *textColor=nullptr,int leftMargin=10){
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoScrollbar;
	ImGui::SetNextWindowPos(ImVec2(x,y));
	ImGui::SetNextWindowSize(ImVec2(width,height));

	string winId = "##winOutputDouble";
	winId.append(to_string(referenceId));

	ImGui::Begin(winId.c_str(),NULL,flags);
	ImGui::SetCursorPosX(leftMargin);
	ImGui::BeginGroup();
	ImGui::PushFont(fonteBold);
	ImGui::Text(label);
	ImGui::Spacing();
	bool del = textColor==nullptr;
	if(del) textColor = new ImVec4(0.3,0.2,0.4,1);

	ImGui::TextColored(*textColor,"%.3f %s ",value,units);
	//if(ImGui::IsItemHovered()) ImGui::SetTooltip("Altura das Barras de Controle");
	ImGui::EndGroup();
	ImGui::PopFont();
	ImGui::End();

	if(del) delete textColor;

}

void inputDoubleWindow(const char* label, const char* units, double *value, int referenceId, int x, int y, int width=100,int height=90, int leftMargin=15,bool * enableCheckbox = nullptr,double step = 0.01){
	        
			ImGuiWindowFlags flags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove |ImGuiWindowFlags_NoScrollbar;
		
			//ImGui::BeginChild("Barras",ImVec2(width,height),flags);
			ImGui::SetNextWindowPos(ImVec2(x,y));
			ImGui::SetNextWindowSize(ImVec2(width,height));

			string winId = "##winInputDouble";
			winId.append(to_string(referenceId));

			ImGui::Begin(winId.c_str(),NULL,flags);
			ImGui::SetCursorPosX(leftMargin);
			ImGui::PushFont(fonteBold);
			ImGui::Text(label);
			//if(ImGui::IsItemHovered()) ImGui::SetTooltip("Altura das Barras de Controle");
			
			ImGui::PushItemWidth(width*0.6);
			string id = "##inDouble";
			id.append(to_string(referenceId));

			ImGui::InputDouble(id.c_str(),value,(0.0),(0.0),"%.3f",ImGuiInputTextFlags_EnterReturnsTrue);
			ImGui::SameLine();
			//ImGui::Spacing();
			ImGui::PopItemWidth();
			ImGui::Text(units);
			ImGui::SetCursorPosX(leftMargin+5);
			if(ImGui::Button("+",ImVec2(20,0))) *value+=0.02;
			ImGui::SameLine();
			if(ImGui::Button("-",ImVec2(20,0))) *value-=0.02;
			if(enableCheckbox!=nullptr){
				string chkId = "Habilitar##";
				chkId.append(to_string(referenceId));
				ImGui::Checkbox(chkId.c_str(),enableCheckbox);
			}
			
			ImGui::PopFont();
			ImGui::End();
}


//Não funciona bem
inline void textXCentered(const char* txt, int areaWidth=DEFAULT_DISPLAY_W){
	float pos = ImGui::GetCursorPosX();
	float width = sizeof(txt)/sizeof(char)*ImGui::GetFontSize();
	if(width>0) 	ImGui::SetCursorPosX(0.4975*areaWidth-0.5f*width);
	else ImGui::SetCursorPosX(0);
	ImGui::Text(txt);
	ImGui::SetCursorPosX(pos);
}

//Centers X cursor and returns text width
inline float centerXForText(const char *text,int windowWidth = DEFAULT_DISPLAY_W){
	float width = sizeof(text)/sizeof(char)*ImGui::GetFontSize();
	ImGui::SetCursorPosX(0.4975f*windowWidth-0.5f*width);
	return width;
}

//Cria um botão centralizado
bool btnCenter(const char* label, int windowWidth=DEFAULT_DISPLAY_W){
	
	return ImGui::Button(label,ImVec2(centerXForText(label,windowWidth),0));
}
	
//Exibe uma mensagem no centro da janela com um botão de ok
void criarModalInfo(const char* titulo, char* mensagem){

    // Calcular o centro da tela
    ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
	//Colocar a prox janela no centro e definir seu pivô para o meio
	//ImGuiCond_Appearing só seta essa variável se a janela estava oculta ou é a 1a vez de sua criação
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	if (ImGui::BeginPopupModal(titulo, NULL, ImGuiWindowFlags_AlwaysAutoResize))
        {
            ImGui::Text(mensagem);
            ImGui::Separator();

			marginLeft(2);
            if (ImGui::Button("OK", ImVec2(50, 0))) { ImGui::CloseCurrentPopup(); }
            ImGui::SetItemDefaultFocus();

            ImGui::EndPopup();
        }
		
}
#endif