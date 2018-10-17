/*
  第一陣（済）：ロボットやプレイヤーをマップ上に一度のみ表示させる。
  慣れない方式であるロボットの座標を配列で管理する方式を混ぜてみたい。
  資料中の(1)と(2)は動的に確保したロボット配列、(3)はフィールド配列を用いることにする。
  しかしこれらの配列を同期させるのが少し難しそうなのが懸念要素である。
  流れとしては、配置をランダマイズしたロボットとプレイヤーを配置し、
  それらを表示して終了することになる。

  第二陣（済）：ゲームループを有効化し、入力関数を作る。
  そして入力に応じたプレイヤーとロボットの動きを実装する。
  入力数値と動きの対応は計算で行う。
  動く先に何かがあれば移動できないようにする．
  問題はロボット同士の衝突の判定になるが、既に動いた個体かどうかの配列を作り，
  既に動いた相手にぶつかった場合衝突するという形をとることにする．

  ->実際に組んでみると、並んで移動するロボットが、
  後ろ側が先に動いた場合後ろ側が姿を消してしまう不具合に直面した。
　これは重なり判定をすることで解決した。

  ->ランダムに散らばらせるとき、一度整列させたロボット間で交換すると、
  ロボットが二重になってフィールドのロボットは残存するという不具合に直面した。
  要するに、抜け殻が残って詰むようになった。
  これはマップの配列を参照して操作していたのが原因だったため、ロボットの配列から
  操作することで解決した。が、入れ替えの処理がやや大きくなってしまい、頭を抱えている。
  
  ->3体が同時に衝突したとき、4点がカウントされていた。
  これは例外処理の条件式に不備があったのが原因だったので、改善した。

  第三陣（未達成）：根幹に関わらない機能をいくつか拡張する。
  （済）スコアを追加する。引数の追加が懸念である。->プレイヤー構造体に入れて解決
  （済）ファイルに書き込んでスコアの保存を行う。
  （済）移動権放棄コマンドの追加(プレイヤー構造体の拡張)
  （断念）早いロボットの追加(もともと処理がうまくまとまっておらず、さらにそれを拡張することになるとひどく手間がかかるため)

*/

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <unistd.h>

// マップサイズ
#define WIDTH 60
#define HEIGHT 20

#define TRUE 1
#define FALSE 0

#define DEBUG 0

typedef struct{
  int x;
  int y;
}point_t;

typedef struct{
  point_t point;
  double score;
  unsigned int is_standing: 1;
}player_t;

typedef struct{
  point_t point;
  unsigned int  is_running: 1;
}robot_t;

typedef struct{
  robot_t* robs;
  int quantity;
}robots_t;

typedef struct{
  unsigned int unit: 2;
}tile_t;

enum UNIT{NONE,ROBOT,SCRAP,PLAYER};

// 外部ファイルの関数
extern char getChar(void);


void set_stage(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player,int stage);
void init_stage(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player,int stage);
void make_robots(robots_t* robots);
void shuffle_units_in_field(tile_t field);
void set_field_material(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player);
void set_field(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player);
void set_random_point(point_t* point);
void process_conflicting(tile_t field[HEIGHT][WIDTH],robots_t* robots,point_t* rand_point,point_t* changing_point,player_t* player);
void swap_tile(tile_t* a,tile_t* b);
int get_robot_num(robots_t* robots,point_t* rand_point);
int is_same_point(point_t* a,point_t* b);
void show_field(tile_t field[HEIGHT][WIDTH],double score,int stage);
char select_letter(tile_t field[HEIGHT][WIDTH],int i,int j);
int select_frame(int i,int j,char* c);
int control_character(tile_t field[HEIGHT][WIDTH],player_t* player,robots_t* robots);
void teleport(tile_t field[HEIGHT][WIDTH],player_t* player,robots_t* robots);
void copy_point(const point_t* source,point_t* target);
int move_player(tile_t field[HEIGHT][WIDTH],player_t* player_p,point_t* tmp_p);
int move_robots(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player);
void halt_robot(tile_t field[HEIGHT][WIDTH],point_t* move_point,robot_t* halted_robot);
void calc_move_point(point_t* move_point,point_t* point_robot,point_t* point_diff);
int judge_crashed(robots_t* robots,point_t* move_point,int* is_moved_yet,double* score,int is_player_standing);
int judge_occupied(robots_t* robots,point_t* former_point,int focused);
int judge_cleared(tile_t field[HEIGHT][WIDTH]);
void create_result(double score,int stage);
//int get_high_score(FILE* fp);
void write_date(FILE* fp);

int main(void)
{
  tile_t field[HEIGHT][WIDTH];
  robots_t robots;
  player_t player;
  int clear_flag=FALSE;
  int die_flag=FALSE;
  int stage=1;

  srandom((unsigned)time(NULL));

  robots.robs=NULL;

  //ゲームループ
  while(TRUE){
    set_stage(field,&robots,&player,stage);

    while(die_flag==FALSE && clear_flag==FALSE){
      show_field(field,player.score,stage);
      while(control_character(field,&player,&robots)==FALSE);
      die_flag=move_robots(field,&robots,&player);
      clear_flag=judge_cleared(field);
    }
    clear_flag=FALSE;
    if(die_flag==TRUE){
      break;
    }else{
      player.score+=stage*10;
      stage++;
    }

  }

  printf("\n\ngame over.\nyour score:%d\n",(int)player.score);
  create_result(player.score,stage);
  
  free(robots.robs);

  return 0;
}

//ゲーム開始/ステージクリア後のフィールド生成を行う
void set_stage(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player,int stage)
{
  init_stage(field,robots,player,stage);
  
  make_robots(robots);
  
  set_field_material(field,robots,player);
  set_field(field,robots,player);

}

// ゲーム要素の初期化
void init_stage(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player,int stage)
{
  int i,j;
  
  for(i=0;i<HEIGHT;i++){
    for(j=0;j<WIDTH;j++){
      field[i][j].unit=NONE;
    }
  }
  if(stage<=7){
    robots->quantity=5*stage;
  }else{
    robots->quantity=40;
  }

  if(stage==1){
    player->score=0;
  }
  player->is_standing=FALSE;
  
}

//robotたちの実体をつくる
void make_robots(robots_t* robots)
{
  int i;

  if(robots->robs!=NULL){
    free(robots->robs);
  }
  robots->robs=(robot_t*)malloc(sizeof(robot_t)*robots->quantity);

  if(robots->robs==NULL){
    printf("failed to allocate.\n");
    exit(EXIT_FAILURE);
  }
  
  for(i=0;i<robots->quantity;i++){
    robots->robs[i].is_running=1;
  }
}

//fieldにunitの散らされていない実体を並べる
void set_field_material(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player)
{
  int i;
  
  field[0][0].unit=PLAYER;
  player->point.x=0;
  player->point.y=0;
  for(i=1;i<=robots->quantity;i++){
    field[i/WIDTH][i%WIDTH].unit=ROBOT;
    robots->robs[i-1].point.x=i%WIDTH;
    robots->robs[i-1].point.y=i/WIDTH;
    
  }
  
}

//fieldからunitをばらばらに散らしつつ、unitに座標を付加する
void set_field(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player)
{
  point_t rand_point;
  point_t changing_point;
  int i;

  for(i=0;i<robots->quantity+1;i++){
    set_random_point(&rand_point);

#if DEBUG
    printf("randx:%d,randy:%d (%d times)\n",rand_point.x,rand_point.y,i);
#endif
    
    if(i==0){
      changing_point=player->point;
    }else{
      changing_point=robots->robs[i-1].point;
    }
    
    process_conflicting(field,robots,&rand_point,&changing_point,player);
    
    swap_tile(&field[changing_point.y][changing_point.x],&field[rand_point.y][rand_point.x]);

#if DEBUG
    show_field(field,0,0);
    getchar();
#endif
    if(i==0){
      player->point=rand_point;
    }else{
      robots->robs[i-1].point=rand_point;
    }
    
  }
}

void set_random_point(point_t* point)
{
  point->x=random()%WIDTH;
  point->y=random()%HEIGHT;
}


//マップ生成でユニットを散らばらせる際、被りが発生した時の処理
void process_conflicting(tile_t field[HEIGHT][WIDTH],robots_t* robots,point_t* rand_point,point_t* changing_point,player_t* player)
{
  int rob_num;

  // ランダムでの移動先に他のユニットがいる場合は、 交換相手の座標を自分の座標に設定しなおす
  if(field[rand_point->y][rand_point->x].unit!=NONE){
#if DEBUG
    printf("conflicting has occured\n");
#endif
    if(field[rand_point->y][rand_point->x].unit==ROBOT){
      // 被った相手のロボットの番号を座標からとり、座標を書き換える
      rob_num=get_robot_num(robots,rand_point);
      if(rob_num==-1){
	exit(EXIT_FAILURE);
      }
      copy_point(changing_point,&(robots->robs[rob_num].point));
    }else{
      copy_point(changing_point,&(player->point));
    }
  }

}


void swap_tile(tile_t* a,tile_t* b)
{
  tile_t tmp=*a;

  *a=*b;
  *b=tmp;
}

//座標からロボット配列での番号を取得
int get_robot_num(robots_t* robots,point_t* rand_point)
{
  int i;

  for(i=0;i<robots->quantity;i++){
    if(is_same_point(&(robots->robs[i].point),rand_point)==TRUE){
      return i;
    }
  }

  return -1;

}

int is_same_point(point_t* a,point_t* b)
{
  if(a->x==b->x && a->y == b->y){
    return TRUE;
  }
  return FALSE;
}


// 指定のロボットの座標を、ループで決めた位置に切り替える
void change_point(const point_t* source,point_t* target)
{
  target->x=source->x;
  target->y=source->y;

}

//フィールドとプレイ状況の表示
void show_field(tile_t field[HEIGHT][WIDTH],double score,int stage)
{
  int i,j;
  char c;

  puts("");
  for(i=-1;i<=HEIGHT;i++){
    for(j=-1;j<=WIDTH;j++){
      c=select_letter(field,i,j);
      printf("%c",c);
    }
    puts("");
  }
  printf("\nyour score:%d\tstage:%d\n",(int)score,stage);

}

//フィールドに何を表示するべきか取得
char select_letter(tile_t field[HEIGHT][WIDTH],int i,int j)
{
  char c;
  
  if(select_frame(i,j,&c)==0){
    switch(field[i][j].unit){
    case NONE:
      c='.';
      break;
    case ROBOT:
      c='+';
      break;
    case SCRAP:
      c='*';
      break;
    case PLAYER:
      c='@';
      break;
    }
  }
      
  return c;

}

//枠ならば文字を設定しながら1を返す
int select_frame(int i,int j,char* c)
{
  if(i==-1 || i==HEIGHT){
    if(j==-1 || j==WIDTH){
      *c='+';
      return TRUE;
    }else{
      *c='-';
      return TRUE;
    }
  }
  if(j==-1 || j==WIDTH){
    *c='|';
    return TRUE;
  }
  
  return FALSE;
}
  

//操作が正常に認められた場合TRUEを返す
int control_character(tile_t field[HEIGHT][WIDTH],player_t* player,robots_t* robots)
{
  int move;
  point_t tmp;
  char c;

  if(player->is_standing==TRUE){
    usleep(100000);
    return TRUE;
  }
  
  printf("操作を入力：");
  
  do{
    c=getChar();
  }while(isdigit(c)==0 && c!='s');

  //移動放棄の処理
  if(c=='s'){
    player->is_standing=TRUE;
  }else{
    move=c-'0';
    if(move==0){
      teleport(field,player,robots);
      return TRUE;
    }else{
      tmp.x=player->point.x+((move-1)%3-1);
      tmp.y=player->point.y+(-1*((move-1)/3-1));
      return move_player(field,player,&tmp);
    }
  }
  
  return FALSE;
}


//プレイヤーの座標をランダムに再設定
void teleport(tile_t field[HEIGHT][WIDTH],player_t* player,robots_t* robots)
{
  int i;
  point_t tmp_point;
  int same_flag;

  field[player->point.y][player->point.x].unit=NONE;
  
  do{
    same_flag=FALSE;
    set_random_point(&tmp_point);

    for(i=0;i<robots->quantity;i++){
      if(is_same_point(&tmp_point,&(robots->robs[i].point))==TRUE){
	same_flag=TRUE;
	break;
      }
    }
    
  }while(same_flag==TRUE);

  copy_point(&tmp_point,&(player->point));
  
  field[player->point.y][player->point.x].unit=PLAYER;
}

void copy_point(const point_t* source,point_t* target)
{
  target->x=source->x;
  target->y=source->y;
}

//移動が問題なくできるかどうかの判定，できれば移動した上でTRUEを返す(できなければFALSE)
int move_player(tile_t field[HEIGHT][WIDTH],player_t* player_p,point_t* tmp_p)
{
  if(tmp_p->y>=0 && tmp_p->y<HEIGHT && tmp_p->x>=0 && tmp_p->x<WIDTH){
    if(field[tmp_p->y][tmp_p->x].unit!=ROBOT && field[tmp_p->y][tmp_p->x].unit!=SCRAP){
      field[player_p->point.y][player_p->point.x].unit=NONE;
      copy_point(tmp_p,&(player_p->point));
      field[player_p->point.y][player_p->point.x].unit=PLAYER;
      return TRUE;
    }
  }
  
  return FALSE;
  
}

//ロボットがプレイヤーを殺したらTRUEを返す
//変数と処理の依存関係から、これ以上の関数化は有効でないと判断した
int move_robots(tile_t field[HEIGHT][WIDTH],robots_t* robots,player_t* player)
{
  int i;
  point_t point_diff;
  point_t move_point;
  int is_crashed;
  int* is_moved_yet=(int*)malloc(sizeof(int)*robots->quantity);
  
  for(i=0;i<robots->quantity;i++){
    is_moved_yet[i]=FALSE;
  }
 
  for(i=0;i<robots->quantity;i++){
    // 駆動しているロボットすべてを調べる
    if(robots->robs[i].is_running==TRUE){
      point_diff.x=player->point.x - robots->robs[i].point.x;
      point_diff.y=robots->robs[i].point.y - player->point.y;

      calc_move_point(&move_point,&(robots->robs[i].point),&point_diff);

      if(is_same_point(&move_point,&(player->point))==TRUE){
	free(is_moved_yet);
	return TRUE;
      }

      is_crashed=judge_crashed(robots,&move_point,is_moved_yet,&(player->score),player->is_standing);

      // 元いた場所に動いてきた僚機がなければクリアして、次の場所を衝突の是非に応じて変える
      if(judge_occupied(robots,&(robots->robs[i].point),i)==FALSE){
	field[robots->robs[i].point.y][robots->robs[i].point.x].unit=NONE;
      }
      if(is_crashed==TRUE){
	// 衝突したらロボットを動かない状態にし、注目している方の分のみスコア加算
	halt_robot(field,&move_point,&(robots->robs[i]));
	if(player->is_standing==FALSE){
	  player->score++;
	}else{
	  player->score+=1.5;
	}
      }else{
	field[move_point.y][move_point.x].unit=ROBOT;
      }

      copy_point(&move_point,&(robots->robs[i].point));

      is_moved_yet[i]=TRUE;
    }
    
  }
  
  free(is_moved_yet);
  return FALSE;
}

//衝突したロボットを始末する
void halt_robot(tile_t field[HEIGHT][WIDTH],point_t* move_point,robot_t* halted_robot)
{
  field[move_point->y][move_point->x].unit=SCRAP;
  halted_robot->is_running=FALSE;
}

// 次に動く座標をロボットとプレイヤーの座標の差から計算する
void calc_move_point(point_t* move_point,point_t* point_robot,point_t* point_diff)
{
  if(point_diff->x<0){
    move_point->x=point_robot->x-1;
  }else if(point_diff->x>0){
    move_point->x=point_robot->x+1;
  }else{
    move_point->x=point_robot->x;
  }
  if(point_diff->y<0){
    move_point->y=point_robot->y+1;
  }else if(point_diff->y>0){
    move_point->y=point_robot->y-1;
  }else{
    move_point->y=point_robot->y;
  }
}

//今注目しているもの以外のロボットの座標を調べ、
//どちらも移動後の結果で同じ座標であれば衝突したとみなしてTRUEを返す
int judge_crashed(robots_t* robots,point_t* move_point,int* is_moved_yet,double* score,int is_player_standing)
{
  int i;
  
  for(i=0;i<robots->quantity;i++){
    if(is_same_point(move_point,&(robots->robs[i].point))==TRUE){
      // 生きているロボット同士の衝突ではスコアをさらにプラスする
      if(is_moved_yet[i]>0){
	if(robots->robs[i].is_running==TRUE){
	  robots->robs[i].is_running=FALSE;
	  if(is_player_standing==FALSE){
	    (*score)++;
	  }else{
	    (*score)+=1.5;
	  }
	}
	return TRUE;
      }
      if(robots->robs[i].is_running==FALSE){
	return TRUE;
      }
    }
  }
  return FALSE;
}

// 元いた座標に移動してきた僚機がいればTRUEを返す
int judge_occupied(robots_t* robots,point_t* former_point,int focused)
{
  int i;

  for(i=0;i<robots->quantity;i++){
    // 自分の座標は除く
    if(i!=focused){
      if(former_point->x == robots->robs[i].point.x && former_point->y == robots->robs[i].point.y){
	return TRUE;
      }
    }
  }
  return FALSE;
}

//駆動中のロボットがいなければTRUEを返す
int judge_cleared(tile_t field[HEIGHT][WIDTH])
{
  int i,j;
  
  for(i=0;i<HEIGHT;i++){
    for(j=0;j<WIDTH;j++){
      if(field[i][j].unit==ROBOT){
	return FALSE;
      }
    }
  }

  return TRUE;
}

//クリア時の記録をファイル出力する
void create_result(double score,int stage)
{
  FILE* fp;
  
  fp=fopen("result.dat","a");
  
  if(fp==NULL){
    printf("\nfailed to save data.\n");
  }else{
    write_date(fp);
    fprintf(fp,"stage:%d\nscore:%d\n\n",stage,(int)score);
  }
    fclose(fp);

}

/*
int get_high_score(FILE* fp)
{
  int high_score=-1;
  int score=-1;
  char buf[255];
  
  while(fgets(buf,255,fp)!=NULL){
    sscanf(buf,"score:%d",&score);
    if(high_score<score){
      high_score=score;
    }
  }

  return high_score;
}
*/

//ファイルに日付の出力
void write_date(FILE* fp)
{
  time_t t;
  struct tm* t_parameter;
  int year,month,day,hour,min,sec;
  
  time(&t);
  t_parameter=localtime(&t);

  year=t_parameter->tm_year+1900;
  month=t_parameter->tm_mon+1;
  day=t_parameter->tm_mday;
  hour=t_parameter->tm_hour;
  min=t_parameter->tm_min;
  sec=t_parameter->tm_sec;
  
  fprintf(fp,"%4d年%d月%d日 %02d:%02d:%02d\n",year,month,day,hour,min,sec);
}

