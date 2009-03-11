/* (c) 2008,2009 Jonathan Hogg and Andreas Grothey, University of Edinburgh
 *
 * This file is part of SML.
 *
 * SML is free software: you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation, using version 3 of the License.
 *
 * SML is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program. If not, see http://www.gnu.org/licenses/.
 */
#ifndef MODEL_INTERFACE_H
#define MODEL_INTERFACE_H

#include <vector>
#include <list>

/** 
 * @class ExpandedModelInterface
 * @brief The representation of a a submodel (block) in the expanded model 
 * which is passed to the solver to allow it to calculate values.
 * 
 * We offer two iterators:
 * child_iterator - iterates over descenants in depth-first order
 * ancestor_iterator - iterates back to root
 * These allow access to all nodes we may interact with.
 * Descdants are submodels contained within this one, and ancestors are
 * models in which this model is contained.
 *
 */
class ExpandedModelInterface {
 friend class AmplModel;
 public:
 std::vector <ExpandedModelInterface*> children;  //!< list of children

 protected:
  /** list of child nodes (vector, so that it can be indexed) */
  ExpandedModelInterface *parent;         //!< parent node 

 public:
  /** Depth first iterator */
  class child_iterator {
   private:
    ExpandedModelInterface *model_;
    std::vector<ExpandedModelInterface*>::iterator itr_;
    child_iterator *desc_itr_;
    bool end_;
   public:
    child_iterator(ExpandedModelInterface *const model, bool end=false) :
      model_(model), itr_(model->children.begin()), desc_itr_(NULL), end_(end)
    {
       if(model->children.end() == itr_) return; // No children
       if(end_) { // We are to signal that we're at the end by desc_itr_=NULL
          itr_ = model->children.end();
          return; 
       }
       desc_itr_ = new child_iterator(*itr_);
    }
    child_iterator& operator=(const child_iterator &other) {
       model_ = other.model_;
       itr_ = other.itr_;
       desc_itr_ = other.desc_itr_;
       return (*this);
    }
    bool operator==(const child_iterator &other) { 
       return ( (other.model_ == model_) &&
                (other.itr_ == itr_) &&
                (other.desc_itr_ == desc_itr_) &&
                (other.end_ == end_) );
    }
    bool operator!=(const child_iterator &other) { return !(*this == other); }
    child_iterator& operator++() {
      if(!desc_itr_) { // Have reached the end
         end_ = true;
         return (*this);
      }
      if(desc_itr_->desc_itr_) { // Easy case, jsut increment descendant
        ++(*desc_itr_);
        return (*this);
      }
      delete desc_itr_; // Done with
      if((++itr_) == model_->children.end()) { // No further children
        desc_itr_ = NULL;
        return (*this);
      }
      desc_itr_ = new child_iterator(*itr_);
      return (*this);
    }
    child_iterator operator++(int) {
      child_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    ExpandedModelInterface* operator*() const {
      if(desc_itr_) return **desc_itr_;
      return model_;
    }
  };

  /** Ancestor iterator */
  class ancestor_iterator {
   private:
    ExpandedModelInterface *model_;
   public:
    ancestor_iterator(ExpandedModelInterface *const model) :
      model_(model) {}
    ancestor_iterator& operator=(const ancestor_iterator &other) {
      model_ = other.model_;
      return (*this);
    }
    bool operator==(const ancestor_iterator &other) { 
      return (other.model_ == model_);
    }
    bool operator!=(const ancestor_iterator &other) { return !(*this == other); }
    ancestor_iterator& operator++() {
      model_ = model_->parent;
      return (*this);
    }
    ancestor_iterator operator++(int) {
      ancestor_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
    ExpandedModelInterface* operator*() const { return model_; }
  };

  child_iterator cbegin() { return child_iterator(this); }
  child_iterator cend() { return child_iterator(this, true); }
  ancestor_iterator abegin() { return ancestor_iterator(parent); }
  ancestor_iterator aend() { return ancestor_iterator(NULL); }
  
  //! Return nb local vars.
  virtual int getNLocalVars() = 0;

  //! Return names of local variables.
  virtual const std::list<std::string>& getLocalVarNames() = 0;

  //! Return nb local cons.
  virtual int getNLocalCons() = 0;

  //! Return names of local constraints.
  virtual const std::list<std::string>& getLocalConNames() = 0;

  //! Returns the nonzeros in the Jacobian of a section of the model.
  virtual int getNzJacobianOfIntersection(ExpandedModelInterface *emcol) = 0;

  //! Returns the nonzeros in the Jacobian of a section of the model.
  virtual void getJacobianOfIntersection(ExpandedModelInterface *emcol, int *colbeg,
				 int *collen, int *rownbs, double *el) = 0;

  //! Returns the vector of lower bounds for the constraints in this model
  virtual void getRowLowBounds(double *elts) = 0;

  //! Returns the vector of upper bounds for the constraints in this model
  virtual void getRowUpBounds(double *elts) = 0;

  //! Returns the vector of lower bounds for the local variables in this model
  virtual void getColLowBounds(double *elts) = 0;

  //! Returns the vector of upper bounds for the local varables in this model
  virtual void getColUpBounds(double *elts) = 0;

  //! Returns the objective gradient for the local model w.r.t. local vars
  virtual void getObjGradient(double *elts) = 0;

  //! Upload the local variable solutions
  virtual void setPrimalSolColumns(double *elts) = 0; 
  
  //! Upload the local variable duals (multipliers on bounds)
  virtual void setDualSolColumns(double *elts) = 0; 

  //! Upload the local constraints slacks
  virtual void setPrimalSolRows(double *elts) = 0; 
  
  //! Upload the local constraints duals (multipliers on constraints)
  virtual void setDualSolRows(double *elts) = 0; 

  //! Returns unique name of this block
  virtual std::string getName() const = 0;

  //! Outputs the solution to the supplied stream at given indent 
  virtual void outputSolution(std::ostream &out, int indent=0) = 0;

 protected:
  virtual ~ExpandedModelInterface() {}
};

#endif
