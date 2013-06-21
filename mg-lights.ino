// This is just a mockup sketch for test scons works

#include "Arduino.h"

int REDPin = 3;    // RED pin of the LED to PWM pin 4
int GREENPin = 5;  // GREEN pin of the LED to PWM pin 5
int BLUEPin = 6;   // BLUE pin of the LED to PWM pin 6
int brightness = 0; // LED brightness
int increment = 5;  // brightness increment
const int buttonPin = 8;     // the number of the pushbutton pin
const int ledPin =  13;      // the number of the LED pin


/*************************/
/* LL-STAR MOTION PARSER */
/*************************/
typedef struct observation {
    int down;
} observation_t;

// Motion parser written in C for LL(*)

typedef int bool_t;
typedef bool_t (*predicate_t)(observation_t);
typedef int terminal_t;
typedef int symbol_t; // terminal_t or nonterminal_t
typedef void (*mutator_t) ();
typedef void* untyped_t;

void assert(bool_t val) {
  if(!val) {
    //digitalWrite(ledPin, HIGH);
  }
}

typedef enum {
  NONTERMINAL, // Simply a nonterminal
  PREDICATE,   // A predicate, should been defined before code generation (in the lisp program)
  MUTATOR,     // A side effect, you're expected to write one c function for each type of mutator
  KLEENE       // Like predicate, but "stay where you are" if true
} symbol_type_t ;

// XXX: Since this is real hardware I decereased these numbers by a lot
#define MAX_PRODUCTION_LENGTH 10
#define MAX_PRODUCTIONS 10
#define STORE_SIZE 20
#define MAX_NODES 10
#define MAX_EDGES 50

typedef struct dfa dfa_t;
typedef struct production production_t;
typedef struct nonterminal nonterminal_t;

typedef struct production {
  int num_symbols;
  untyped_t data[MAX_PRODUCTION_LENGTH]; // Either (nonterminal_t, predicate_t, mutator_t)
  symbol_type_t types[MAX_PRODUCTION_LENGTH];
} production_t;

typedef struct nonterminal {
  //int num_productions;
  //production_t *productions[MAX_PRODUCTIONS]; // We don't need this probably
  dfa_t *automata;
} nonterminal_t;

observation_t store[STORE_SIZE];

int curr_beg; // From where we know our observations;
int curr_end; // Position of the end of store

bool_t queue_is_empty () {
  return curr_beg == curr_end;
}

extern observation_t make_observation();

observation_t fetch_observation() {
  assert(curr_beg != (curr_end+1)%STORE_SIZE && "Ran out of memory, queue size is too small");
  observation_t ret = store[curr_end] = make_observation();
  curr_end = (curr_end+1)%STORE_SIZE;
  return ret;
}

observation_t get_observation(int pos, bool_t *is_fetched_observation) {
  if (pos == curr_end) {
    if (is_fetched_observation != NULL) *is_fetched_observation = 1;
    return fetch_observation();
  } else {
    // assuming it's in range here, namely "pos < curr_end" (but in modulo and such)
    if (is_fetched_observation != NULL) *is_fetched_observation = 0;
    return store[pos];
  }
}

/// handle_mutator: Perform the action if it's the first time we observe it
void handle_mutator(mutator_t mutator, bool_t is_fetched_observation) {
  if(is_fetched_observation) {
    mutator();
  }
}

typedef struct dfa {
  int num_nodes;
  int start_node;
  production_t *final[MAX_NODES]; // If null then not a accept state. Otherwise predict to given production
  int range[MAX_NODES+1]; // the edges for node n are in the span [range[n], range[n+1])
  untyped_t edges[MAX_EDGES]; // predicate ==> if it turns true then take this edge. mutator ==> perform if new and always take edge
  symbol_type_t types[MAX_EDGES]; // The type of the edge. (Should only be mutator or predicate)
  int destinations[MAX_EDGES]; // by following edge j you end up at destinations[j]
} dfa_t;

production_t *run_dfa(dfa_t *automata) {
  int pred_pos = curr_beg;
  int curr_node = automata->start_node;
  while (1) {
    if(automata->final[curr_node]) {
      return automata->final[curr_node];
    }
    bool_t is_fetched_observation;
    observation_t obs = get_observation(pred_pos, &is_fetched_observation);
    int beg = automata->range[curr_node], end = automata->range[curr_node+1];
    int happened = 0; // Just for sanity check
    for(int i = beg; i < end; i++) {
      switch( automata->types[i] ) {
        case MUTATOR: {
          mutator_t mutator = (mutator_t) automata->edges[i];
          handle_mutator(mutator, is_fetched_observation);
          assert((beg+1 == end) && "We only expect one edge if there's a mutator");
          happened = 1;
                      }
          break;
        case PREDICATE: {
          predicate_t pred = (predicate_t) automata->edges[i];
          happened = pred(obs);
                        }
          break;
        default: assert(0 && "Invalid type");
      }
      if (happened) {
        pred_pos = (pred_pos+1)%STORE_SIZE;
        curr_node = automata->destinations[i];
        break;
      }
    }
    assert(happened); // Syntax error!
  }
}

void run_nonterminal(nonterminal_t *nt) {
  production_t *prod;
beginning:
  prod = run_dfa(nt->automata);
  for(int i = 0; i < prod->num_symbols; i++ ) {
    switch( prod->types[i] ) {
      case NONTERMINAL:
        if(i == prod->num_symbols-1) {
          nt = (nonterminal_t*) prod->data[i];
          goto beginning; // TCO
        }
        else {
          run_nonterminal((nonterminal_t*) prod->data[i]);
        }
        break;
      case KLEENE: {
        predicate_t pred = (predicate_t) prod->data[i];
        observation_t obs = get_observation(curr_beg, NULL);
        if(pred(obs)) {
          i--;
        }
        curr_beg = (curr_beg+1)%STORE_SIZE;
                   }
        break;
      case PREDICATE: {
        predicate_t pred = (predicate_t) prod->data[i];
        observation_t obs = get_observation(curr_beg, NULL);
        assert(pred(obs) && "Syntax error OR bug in dfa/prediction");
        curr_beg = (curr_beg+1)%STORE_SIZE;
                      }
        break;
      case MUTATOR: {
        mutator_t mutator = (mutator_t) prod->data[i];
        bool_t is_fetched_observation;
        get_observation(curr_beg, &is_fetched_observation);
        handle_mutator(mutator, is_fetched_observation);
        curr_beg = (curr_beg+1)%STORE_SIZE;
                    }
        break;
    }
  }
}
bool_t predicate__lp_not_sp_down_rp_ (observation_t obs) {
return !(obs.down);
}
bool_t predicate_down (observation_t obs) {
return obs.down;
}
bool_t predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_ (observation_t obs) {
return (!(obs.down)) && (!(obs.down));
}

// XXX: go is a nice helper function. It includes a sleep phase
void go(int red, int green, int blue) {
  red = constrain(red, 0, 255);
  green = constrain(green, 0, 255);
  blue = constrain(blue, 0, 255);
    
  analogWrite(REDPin, red);
  analogWrite(GREENPin, green);
  analogWrite(BLUEPin, blue);
  delay(1000);
}

void mutator_yellow () {
  go(120, 80, 0); // XXX
}
void mutator_red () {
  go(80, 0, 0); // XXX
}
void mutator_blu () {
  go(0, 0, 40); // XXX
}
void mutator_strong_red () {
  go(255, 0, 0); // XXX
}
void mutator_strong_yellow () {
  go(255, 150, 0); // XXX
}
void mutator_strong_blu () {
  go(0, 0, 160); // XXX
}

observation_t make_observation () {
  return observation_t { digitalRead(buttonPin) == HIGH }; // XXX
}

extern nonterminal_t nonterminal_b;
extern nonterminal_t nonterminal_r;
extern nonterminal_t nonterminal_loops;
extern nonterminal_t nonterminal_prod;
extern nonterminal_t nonterminal_start;
production_t production_0 = { 3, { (untyped_t)&mutator_yellow, (untyped_t)&nonterminal_prod, (untyped_t)&nonterminal_start }, { MUTATOR, NONTERMINAL, NONTERMINAL } };// Belonging to nonterminal START
production_t production_1 = { 6, { (untyped_t)&predicate_down, (untyped_t)&nonterminal_loops, (untyped_t)&predicate_down, (untyped_t)&mutator_red, (untyped_t)&predicate__lp_not_sp_down_rp_, (untyped_t)&nonterminal_r }, { PREDICATE, NONTERMINAL, PREDICATE, MUTATOR, PREDICATE, NONTERMINAL } };// Belonging to nonterminal PROD
production_t production_2 = { 6, { (untyped_t)&predicate_down, (untyped_t)&nonterminal_loops, (untyped_t)&predicate__lp_not_sp_down_rp_, (untyped_t)&mutator_blu, (untyped_t)&predicate__lp_not_sp_down_rp_, (untyped_t)&nonterminal_b }, { PREDICATE, NONTERMINAL, PREDICATE, MUTATOR, PREDICATE, NONTERMINAL } };// Belonging to nonterminal PROD
production_t production_3 = { 5, { (untyped_t)&predicate_down, (untyped_t)&mutator_red, (untyped_t)&predicate_down, (untyped_t)&mutator_blu, (untyped_t)&nonterminal_loops }, { PREDICATE, MUTATOR, PREDICATE, MUTATOR, NONTERMINAL } };// Belonging to nonterminal LOOPS
production_t production_4 = { 0, {  }, {  } };// Belonging to nonterminal LOOPS
production_t production_5 = { 1, { (untyped_t)&predicate__lp_not_sp_down_rp_ }, { PREDICATE } };// Belonging to nonterminal PROD
production_t production_6 = { 4, { (untyped_t)&mutator_strong_red, (untyped_t)&mutator_strong_yellow, (untyped_t)&nonterminal_prod, (untyped_t)&mutator_strong_red }, { MUTATOR, MUTATOR, NONTERMINAL, MUTATOR } };// Belonging to nonterminal R
production_t production_7 = { 4, { (untyped_t)&mutator_strong_blu, (untyped_t)&mutator_strong_yellow, (untyped_t)&nonterminal_prod, (untyped_t)&mutator_strong_blu }, { MUTATOR, MUTATOR, NONTERMINAL, MUTATOR } };// Belonging to nonterminal B
dfa_t automata_b = { 2, 1, { &production_7, NULL }, { 0, 0, 1 }, { (untyped_t)&mutator_strong_blu }, { MUTATOR }, { 0 } };
dfa_t automata_r = { 2, 1, { &production_6, NULL }, { 0, 0, 1 }, { (untyped_t)&mutator_strong_red }, { MUTATOR }, { 0 } };
dfa_t automata_loops = { 5, 2, { &production_3, &production_4, NULL, NULL, NULL }, { 0, 0, 0, 2, 3, 5 }, { (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_, (untyped_t)&predicate_down, (untyped_t)&mutator_red, (untyped_t)&predicate_down, (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_ }, { PREDICATE, PREDICATE, MUTATOR, PREDICATE, PREDICATE }, { 1, 3, 4, 0, 1 } };
dfa_t automata_prod = { 9, 3, { &production_1, &production_2, &production_5, NULL, NULL, NULL, NULL, NULL, NULL }, { 0, 0, 0, 0, 2, 4, 5, 7, 8, 10 }, { (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_, (untyped_t)&predicate_down, (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_, (untyped_t)&predicate_down, (untyped_t)&mutator_red, (untyped_t)&predicate_down, (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_, (untyped_t)&mutator_blu, (untyped_t)&predicate__lp_and_sp__lp_not_sp_down_rp__sp__lp_not_sp_down_rp__rp_, (untyped_t)&predicate_down }, { PREDICATE, PREDICATE, PREDICATE, PREDICATE, MUTATOR, PREDICATE, PREDICATE, MUTATOR, PREDICATE, PREDICATE }, { 2, 4, 1, 5, 6, 7, 0, 8, 1, 5 } };
dfa_t automata_start = { 2, 1, { &production_0, NULL }, { 0, 0, 1 }, { (untyped_t)&mutator_yellow }, { MUTATOR }, { 0 } };
nonterminal_t nonterminal_b = { &automata_b };
nonterminal_t nonterminal_r = { &automata_r };
nonterminal_t nonterminal_loops = { &automata_loops };
nonterminal_t nonterminal_prod = { &automata_prod };
nonterminal_t nonterminal_start = { &automata_start };
nonterminal_t *start_nonterminal = &nonterminal_start;

void setup(void)
{
  pinMode(ledPin, OUTPUT);
  pinMode(REDPin, OUTPUT);
  pinMode(GREENPin, OUTPUT);
  pinMode(BLUEPin, OUTPUT);
  pinMode(buttonPin, INPUT);
  digitalWrite(ledPin, LOW);
}

void loop(void)
{
  // digitalWrite(ledPin, HIGH);
  run_nonterminal( start_nonterminal );
  // digitalWrite(PIN_LED, HIGH);
  // delay(5000);
  // digitalWrite(PIN_LED, LOW);
  // delay(1000);
}

/* vim: set sw=2 et: */
