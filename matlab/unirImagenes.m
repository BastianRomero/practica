clearvars;

tic
origen = 'files1';
directorioCarga = dir(origen);
directorioNuevo = 'maxfps';
formatoOrigen = 'tif';
formato = '.jpg';
frames = 15;

if (exist(directorioNuevo,'dir') == 7)
    fprintf("El directorio ya existe \n")
else
    mkdir(directorioNuevo)
    fprintf("Directorio creado \n")
end

%% Función de conversion 
fprintf("Convirtiendo imagenes \n")
for numero = 3:length(directorioCarga)
    
   directorio = [origen '/' directorioCarga(numero).name];
   imagen =  imread(directorio);
   if(numero/10 < 1)
       imwrite(imagen,[directorioNuevo '/00' mat2str(numero) formato]);   
   elseif (numero/10 >= 1 && numero/10 < 10)
       imwrite(imagen,[directorioNuevo '/0' mat2str(numero) formato]);  
   elseif (numero/10 >= 10)
       imwrite(imagen,[directorioNuevo '/' mat2str(numero) formato]);   
   end
   
end

%% Creación de video
fprintf("Creando Video \n")
imageNames = dir(fullfile(directorioNuevo,'*.jpg'));
imageNames = {imageNames.name}';
outputVideo = VideoWriter(fullfile(directorioNuevo,'camara.avi'));
outputVideo.FrameRate = frames;
open(outputVideo)

for ii = 1:length(imageNames)
   img = imread(fullfile(directorioNuevo,imageNames{ii}));
   writeVideo(outputVideo,img)
end

close(outputVideo)

toc
fprintf("Programa Terminado \n")
 