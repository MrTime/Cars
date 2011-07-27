uniform sampler2D indexMap;
uniform sampler2D map;

uniform vec2 index_steps;
uniform vec2 steps;

void main(void)
{
   vec2 index_step = vec2(1.0,1.0) / index_steps;
   vec2 step = vec2(1.0) / steps;
   
   vec2 sector_coord = floor(gl_TexCoord[0].st / index_step);
   vec2 sector_start = sector_coord * index_step;
   vec2 in_sector_coord = (gl_TexCoord[0].st - sector_start) * index_steps;
   
   in_sector_coord = clamp(in_sector_coord, vec2(0.0), vec2(1.0 - 6.0/256.0));
   
   vec4 index = texture2D(indexMap, gl_TexCoord[0].st);
   
   // rotate texture according to index.z
   // simulate following code
   // Z coord is codded as following:
   //   z = x_inv * 0.5 + y_inv * 0.25
   // where x_inv & y_inv may be 0 or 1
   //float x_inv = floor(index.z / 0.5);
   //if (x_inv > 0.0) in_sector_coord.x = 1.0 - in_sector_coord.x;
   //if (index.z - x_inv > 0.0) in_sector_coord.y = 1.0 - in_sector_coord.y;
   vec2 inv_coef;
   inv_coef.s = floor(index.z / 0.5);
   inv_coef.t = (index.z - inv_coef.s * 0.5) / 0.25;    
   vec2 not_inv_coef = abs(inv_coef - vec2(1.0)); 
   // r = x * (1.0 - a) + y * a
   // not_inv_coef = inv_coef * (1.0 - in_sector_coord) + not_inv_coef * in_sector_coord   
   in_sector_coord = mix(inv_coef,not_inv_coef,in_sector_coord);         
          
   vec2 map_coord = index.xy + in_sector_coord * step; 
     
   vec4 color =  texture2D(map,  map_coord);   
   gl_FragColor = color;
   //gl_FragColor = vec4(in_sector_coord, 0.0, 1.0);
}