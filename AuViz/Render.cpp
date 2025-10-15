#include "Render.h"

namespace avRender {
	GLuint texturemap = 0, heightmap = 0;
	uint32_t mbands = 0;
	bool rays = false;
	sf::Vector2f rayOPoint{0, 0};
	bool close = false;
	std::unique_ptr<sf::RenderWindow> rwin;
	std::unique_ptr<sf::Texture> gridtex, cleartex;
	std::unique_ptr<sf::RenderTexture> framebuf;
	std::unique_ptr<sf::Shader> mapshader, dbshader, postshader, clearshader; //only fragment shaders
	std::unique_ptr<sf::Transform> gridTransform;
	std::unique_ptr<sf::RectangleShape> viewPort;
	std::unique_ptr<sf::Sprite> tsprt;

	void GLAPIENTRY
		MessageCallback(GLenum source,
			GLenum type,
			GLuint id,
			GLenum severity,
			GLsizei length,
			const GLchar* message,
			const void* userParam)
	{
		fprintf(stderr, "GL CALLBACK: %s type = 0x%x, severity = 0x%x, message = %s\n",
			(type == GL_DEBUG_TYPE_ERROR ? "** GL ERROR **" : ""),
			type, severity, message);
	}

	class button {
		sf::Texture texture;
		sf::CircleShape shape = sf::CircleShape();
		bool clicked = false;
		inline static int occupiedId, occupiedCount;
		unsigned int id;
	public:
		bool drawAndCheck(sf::RenderWindow* wnd) {
			wnd->draw(shape);
			if (occupiedId != id && occupiedId != 0)
				return false;
			if (shape.getGlobalBounds().contains((sf::Vector2f)sf::Mouse::getPosition() - (sf::Vector2f)wnd->getPosition()) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left) && !clicked) {
				clicked = true;
				return true;
			}
			if(!sf::Mouse::isButtonPressed(sf::Mouse::Button::Left))
				clicked = false;
			return false;
		}

		bool drawAndCheckHeld(sf::RenderWindow* wnd) {
			wnd->draw(shape);
			if (occupiedId != id && occupiedId != 0)
				return false;
			if (shape.getGlobalBounds().contains((sf::Vector2f)sf::Mouse::getPosition() - (sf::Vector2f)wnd->getPosition())) {
				shape.setOutlineThickness(2);
				clicked = true;
			}
			else
				shape.setOutlineThickness(0);
			if (clicked && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)) {
				occupiedId = id;
				return true;
			}
			else {
				clicked = false;
				occupiedId = 0;
			}
			return shape.getLocalBounds().contains((sf::Vector2f)sf::Mouse::getPosition() - (sf::Vector2f)wnd->getPosition()) && sf::Mouse::isButtonPressed(sf::Mouse::Button::Left);
		}

		sf::Vector2i getPos() {
			return (sf::Vector2i)(shape.getPosition() + sf::Vector2f(shape.getRadius(), shape.getRadius()));
		}

		button(sf::Vector2f pos, float radius, std::string textureDir)
		: texture(textureDir)
		, shape(radius)
		{
			shape.setTexture(&texture);
			shape.setPosition(pos);
			shape.setOutlineColor(sf::Color::White);
			id = ++occupiedCount;
		}
		
	} borderBtn(sf::Vector2f(100, 500), 20, "../auvizmove.png"),
		offBtn(sf::Vector2f(100, 542), 20, "../auvizoff.png"),
		targetBtn(sf::Vector2f(2, 2), 20, "../auviztarget.png"),
		raysBtn(sf::Vector2f(100, 626), 20, "../auvizrays.png");


	sf::RenderWindow* init(uint32_t bands) {


		mbands = bands;

		rwin = std::make_unique<sf::RenderWindow>();

		settingswin = std::make_unique<sf::RenderWindow>();

		gridtex = std::make_unique<sf::Texture>();

		(*gridtex).loadFromFile("../grid.png");
		auto gts = (*gridtex).getSize();

		(*rwin).create(sf::VideoMode({ 800, 800 }), "AuViz", sf::Style::None);

		settingswin->create(sf::VideoMode({ 44, 44 }), "AuViz ray target", sf::Style::None);
		settingswin->setPosition(rwin->getPosition() + sf::Vector2i(98, 582));

		(*rwin).setActive(true);

		gladLoadGLLoader(reinterpret_cast<GLADloadproc>(sf::Context::getFunction));

		glEnable(GL_DEBUG_OUTPUT);
		glDebugMessageCallback(MessageCallback, 0);

		gridTransform = std::make_unique<sf::Transform>();

		viewPort = std::make_unique<sf::RectangleShape>();

		framebuf = std::make_unique<sf::RenderTexture>();

		cleartex = std::make_unique<sf::Texture>();

		clearshader = std::make_unique<sf::Shader>();
		postshader = std::make_unique<sf::Shader>();
		mapshader = std::make_unique<sf::Shader>();
		dbshader = std::make_unique<sf::Shader>();

		(*rwin).setVerticalSyncEnabled(true);

		(*cleartex).resize({ gts.x, gts.y });

		(*rwin).setFramerateLimit(165);

		glGenTextures(1, &texturemap);
		glGenTextures(1, &heightmap);

		glBindTexture(GL_TEXTURE_2D, texturemap);
		glTextureStorage2D(texturemap, 1, GL_RGBA32F, gts.x, gts.y);

		glBindTexture(GL_TEXTURE_2D, heightmap);
		glTextureStorage2D(heightmap, 1, GL_R32F, bands, gts.y + 1);

		glTextureParameteri(heightmap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(heightmap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

		glBindImageTexture(0, texturemap, 0, 0, 0, GL_READ_WRITE, GL_RGBA32F);
		glBindImageTexture(1, heightmap, 0, 0, 0, GL_READ_WRITE, GL_R32F);

		(*framebuf).resize({ gts.x, gts.y });


		(*viewPort).setSize(sf::Vector2f(gts.x, gts.y)); //the extra 300 is there because sfml doesn't like newer opengl versions
		(*viewPort).setFillColor(sf::Color::Red);
		(*viewPort).setPosition({ 0, (*rwin).getSize().y - (*viewPort).getSize().y });

		(*mapshader).loadFromMemory("#version 460 \n"\
			"uniform layout(binding = 0, rgba32f)image2D mapimg; \n"\
			"uniform layout(binding = 1, r32f)image2D heightmap; \n"\
			"out vec4 FragColor; \n"\
			"uniform sampler2D gridtex; \n"\
			"uniform float fovslope; \n"\
			"uniform vec2 gridmotion; \n"\
			"uniform mat4 spin; \n"\
			"void main(){ \n"\
			"ivec2 viewportResolution = textureSize(gridtex, 0); \n"\
			"vec2 hmRes = imageSize(heightmap);\n"\
			"float c1 = imageLoad(heightmap, ivec2(gl_FragCoord.xy * hmRes / viewportResolution)).r * 10;\n"\
			"float c2 = imageLoad(heightmap, ivec2(gl_FragCoord.xy * hmRes / viewportResolution) + ivec2(1, 0)).r * 10;\n"\
			"float h = mix(c1, c2, hmRes.x * (int(gl_FragCoord.x) % int(viewportResolution.x / hmRes.x)) / viewportResolution.x)*5;\n"\
			"vec3 pixelInWorldSpace = (spin * vec4(gl_FragCoord.xy, clamp(h, 0, 150), 1.0)).xyz; \n"\
			"vec4 gridTexel = texture(gridtex, mod(gl_FragCoord.xy - gridmotion * viewportResolution,viewportResolution) / viewportResolution); \n"\
			"vec2 v = vec2(viewportResolution / 2.0); \n"\
			"vec2 b = vec2(pixelInWorldSpace.xz); \n"\
			"float a = pixelInWorldSpace.y + viewportResolution.y / 2.0; \n"\
			"vec2 t = vec2((b-v)/a);\n"\
			"ivec2 pixelInScreenSpace = ivec2((t / (fovslope/2))*viewportResolution); \n"\
			"if(length(gridTexel.rgb) < 0.5)return; \n"\
			"memoryBarrier();\n"\
			"float pl = imageLoad(mapimg, pixelInScreenSpace / 2 + viewportResolution / 2).a;\n"\
			"if(length(pixelInWorldSpace) <= pl || pl == 0.0)\n"\
			"imageStore(mapimg, pixelInScreenSpace / 2 + viewportResolution / 2, vec4(gridTexel.rgb * (pixelInWorldSpace.z / 100.0 + 0.7), length(pixelInWorldSpace))); \n"\
			"memoryBarrier();\n"\
			"FragColor = vec4(0,0,0,0); \n"\
			"}\0",
			sf::Shader::Type::Fragment);

		(*postshader).loadFromMemory("#version 460\n" \
			"uniform layout(binding = 0, rgba32f) readonly image2D mapimg;\n"\
			"uniform vec2 raysOrigin;"
			"uniform bool raysamples;"
			"out vec4 FragColor;\n"\
			"uniform ivec2 viewportResolution;\n"\
			"void main(){\n"\
			"vec4 accColor = vec4(0);\n"\
			"vec2 sz = imageSize(mapimg);\n"\
			"vec4 ncolor = imageLoad(mapimg, ivec2(gl_FragCoord.xy));\n"\
			"if(raysamples)\n"\
			"for(int i = 0; i < 100; i++)\n"\
			"accColor += imageLoad(mapimg, ivec2(normalize(raysOrigin - gl_FragCoord.xy) * vec2(1, -1) * sz * i / 1600 + gl_FragCoord.xy)) / 10;\n"\
			"FragColor = ncolor + vec4(accColor.rgb, accColor.a / 10000);\n"\
			"}\0",
			sf::Shader::Type::Fragment);

		(*clearshader).loadFromMemory("#version 460\n"\
			"uniform layout(binding = 0, rgba32f) image2D mapimg;\n"\
			"void main(){"
			"imageStore(mapimg, ivec2(gl_FragCoord.xy), vec4(0,0,0,0));"
			"}\n",
			sf::Shader::Type::Fragment);

		(*dbshader).loadFromMemory("#version 460\n"\
			"uniform layout(binding = 1, r32f) image2D heightmap;\n"\
			"uniform int eltime;\n"\
			"uniform float pspeed;\n"\
			"uniform int premov;"
			"void main(){\n"\
			"if(gl_FragCoord.y == 0 || gl_FragCoord.x > imageSize(heightmap).x) discard;"
			"float c = imageLoad(heightmap, ivec2(gl_FragCoord.xy)).r;\n"\
			"vec4 p = imageLoad(heightmap, ivec2(gl_FragCoord.x, 1));\n"\
			"float t = imageLoad(heightmap, ivec2(gl_FragCoord.x, 0)).r;\n"\
			"memoryBarrier();\n"\
			"imageStore(heightmap, ivec2(gl_FragCoord.xy) + ivec2(0, eltime * pspeed), vec4(c, 0, 0, 0));\n"\
			"if(gl_FragCoord.y > eltime * pspeed) return;\n"\
			"vec4 prpx = vec4(mix(t, c, (gl_FragCoord.y) / (eltime * pspeed)), 0, 0, 0);"
			"if(gl_FragCoord.y == 1) prpx = p;\n"\
			"imageStore(heightmap, ivec2(gl_FragCoord.xy), prpx);\n"\
			"}\n", 
			sf::Shader::Type::Fragment);
		
		(*gridTransform).translate({ 0, 300 });
		return rwin.get();
	}
	float ran = false;
	float gm = 0;
	int premov = 0;
	void render(float fovslope, std::int32_t etm, std::vector<float>& in) {
		
		while (const std::optional ev = (*rwin).pollEvent())
			if (ev->is<sf::Event::Closed>()) {
				(*rwin).close();
				close = true; 
				return;
			}
		//in = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
		gm = (int)(++gm) % (int)(*viewPort).getSize().x;

		//std::cout << in[10] << ' ';

		(*gridTransform).rotate(sf::degrees(0.1), sf::Vector2f((*gridtex).getSize()) / 2.f);
		(*mapshader).setUniform("spin", sf::Glsl::Mat4(*gridTransform));
		(*mapshader).setUniform("gridmotion", sf::Vector2f(0, gm / (*gridtex).getSize().y / 2));
		(*mapshader).setUniform("gridtex", *gridtex);
		(*mapshader).setUniform("fovslope", fovslope);
		(*dbshader).setUniform("eltime", etm);
		(*dbshader).setUniform("pspeed", 0.5f);
		(*postshader).setUniform("raysamples", rays);
		(*postshader).setUniform("raysOrigin", (sf::Vector2f)(settingswin->getPosition() - rwin->getPosition() - sf::Vector2i(0, gridtex->getSize().y / 2)));
		glTextureSubImage2D(heightmap, 0, 0, 0, mbands, 1, GL_RED, GL_FLOAT, in.data());
		(*framebuf).draw(*viewPort, dbshader.get());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		(*framebuf).draw(*viewPort, clearshader.get());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		(*framebuf).draw(*viewPort, mapshader.get());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		(*framebuf).draw(*viewPort, postshader.get());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		(*rwin).clear(sf::Color::Transparent);
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
		(*rwin).draw(*viewPort, postshader.get());
		glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl)) {
			if (borderBtn.drawAndCheckHeld(rwin.get()))
				rwin->setPosition(sf::Mouse::getPosition() - borderBtn.getPos());
			if (offBtn.drawAndCheck(rwin.get()))
				close = true;
			if (raysBtn.drawAndCheck(rwin.get()))
				rays = !rays;
			settingswin->setActive(true);
			settingswin->clear(sf::Color::Transparent);
			settingswin->setVisible(true);
			while(settingswin->pollEvent());
			if (targetBtn.drawAndCheckHeld(settingswin.get()))
				settingswin->setPosition(sf::Mouse::getPosition() - targetBtn.getPos());
			settingswin->display();
			rwin->setActive(true);
		}
		else {
			settingswin->setVisible(false);
		}

		(*rwin).display();
	}
	void exit() {
		rwin.release();
		gridtex.release();
		viewPort.release();
		framebuf.release();
		mapshader.release();
		postshader.release();
		settingswin.release();
		gridTransform.release();
	}
}